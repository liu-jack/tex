<?php

namespace mfw
{

require_once 'MfwPacket.php';
require_once 'Query.php';

class MfwEndpoint
{
	private $desc;
	private $protocol;
	private $host;
	private $port;

	public function __construct($desc)
	{
		$this->desc = $desc;

		$v = array_values(array_filter(explode(" ", $desc), function ($var) { return $var != ""; }));
		$n = count($v);
		if ($n == 0 || $n % 2 != 1)
		{
			throw new \Exception('invalid endpoint desc: ' . $desc);
		}

		$this->protocol = $v[0];
		for ($i = 1; $i < $n; $i += 2)
		{
			$key = $v[$i];
			$val = $v[$i + 1];
			if ($key == '-h')
			{
				$this->host = $val;
			}
			else if ($key == '-p')
			{
				$this->port = $val;
			}
		}
	}

	public function getDesc() { return $this->desc; }
	public function getProtocol() { return $this->protocol; }
	public function getHost() { return $this->host; }
	public function getPort() { return $this->port; }

}

class MfwTransport
{
	private $ep;
	private $sock;
	private $iserr;

	public function __construct($ep)
	{
		$this->ep = $ep;
		$this->sock = null;
		$this->iserr = false;
	}

	public function __destruct()
	{
		if ($this->sock)
		{
			fclose($this->sock);
		}
	}

	public function isError()
	{
		return $this->iserr;
	}

	public function connect($iTimeoutMS, &$bIsTimeout)
	{
		if ($this->sock == null)
		{
			$bIsTimeout = false;
			$remote = $this->ep->getProtocol() . '://' . $this->ep->getHost() . ':' . $this->ep->getPort();
			$errno = null;
			$errstr = null;
			$this->sock = stream_socket_client($remote, $errno, $errstr, $iTimeoutMS / 1000);
			if (!$this->sock)
			{
				if ($errno == 110)
				{
					$bIsTimeout = true;
				}
				$this->closeSetError();
				return -1;
			}
			$this->iserr = false;
		}
		return 0;
	}

	public function invoke($sReqData, &$sRspData, $iTimeoutMS)
	{
		$endtime = gettimeofday(true) + $iTimeoutMS / 1000;

		$sec = intval($iTimeoutMS / 1000);
		$usec = $iTimeoutMS % 1000 * 1000;
		stream_set_timeout($this->sock, $sec, $usec);

		$sReqData = pack('N', strlen($sReqData) + 4) . $sReqData;
		$ret = fwrite($this->sock, $sReqData);
		if ($ret === false || $ret != strlen($sReqData))
		{
			$this->closeSetError();
			return -1;
		}
		$info = stream_get_meta_data($this->sock);
		if ($info['timed_out'])
		{
			$this->closeSetError();
			return -1;
		}

		$sRspData = '';
		do
		{
			$now = gettimeofday(true);
			$iTimeoutMS = round(($endtime > $now ? $endtime - $now : 0) * 1000);
			$sec = intval($iTimeoutMS / 1000);
			$usec = $iTimeoutMS % 1000 * 1000;
			stream_set_timeout($this->sock, $sec, $usec);
				
			$ret = fread($this->sock, 8192);
			if ($ret === false)
			{
				$this->closeSetError();
				return -1;
			}
			$info = stream_get_meta_data($this->sock);
			if ($info['timed_out'])
			{
				$this->closeSetError();
				return -1;
			}
				
			$sRspData .= $ret;
			$len = strlen($sRspData);
			if ($len >= 4)
			{
				$arrn = unpack('N', $sRspData);
				$n = $arrn[1];
				if ($n > 100 * 1024 * 1024)
				{
					$this->closeSetError();
					return -1;
				}
				if ($len >= $n)
				{
					$this->iserr = false;
					$sRspData = substr($sRspData, 4, $n - 4);
					return 0;
				}
			}
		} while (!feof($this->sock));

		$this->closeSetError();
		return -1;
	}

	private function closeSetError()
	{
		if ($this->sock)
		{
			fclose($this->sock);
			$this->sock = null;
		}
		$this->iserr = true;
	}
}

class MfwClient
{
	private $sServiceObj;
	private $sDivision;
	private $sServiceName;
	private $vServiceEndpoint;
	private $mTransport;
	private $iConnectTimeoutMS;
	private $iSyncCallTimeoutMS;
	
	private static $sLocator = null;
	public static function setLocator($sLocator)
	{
		self::$sLocator = $sLocator;
	}

	public function __construct($sServiceObj, $sDivision = '')
	{
		$this->iConnectTimeoutMS = 2000;
		$this->iSyncCallTimeoutMS = 5000;

		$this->sServiceObj = $sServiceObj;
		$this->sDivision = $sDivision;
		
		$pos = strpos($sServiceObj, '@');
		if ($pos === false)
		{
			$this->sServiceName = $sServiceObj;
			
			$locator = self::$sLocator;
			if (!$locator)
			{
				$locator = get_cfg_var('mfw.locator');
				if ($locator === false)
				{
					throw new \Exception("missing php.ini config: mfw.locator");
				}
			}
			
			$queryClient = new \mfw\Query($locator);
			$vActiveEps = null;
			$vInactiveEps = null;
			$ret = $queryClient->getEndpoints($sServiceObj, $sDivision, $vActiveEps, $vInactiveEps);
			if ($ret != 0 || count($vActiveEps) == 0)
			{
				throw new \Exception("cannot resolve from registry: $sServiceObj");
			}
			for ($i = 0, $n = count($vActiveEps); $i < $n; ++$i)
			{
				$this->vServiceEndpoint[] = new MfwEndpoint($vActiveEps[$i]);
			}
		}
		else
		{
			$this->sServiceName = substr($sServiceObj, 0, $pos);
			$vEndpoints = explode(':', substr($sServiceObj, $pos + 1));
			for ($i = 0, $n = count($vEndpoints); $i < $n; ++$i)
			{
				if ($vEndpoints[$i] != "")
				{
					$this->vServiceEndpoint[] = new MfwEndpoint($vEndpoints[$i]);
				}
			}
		}
		$this->mTransport = array();
	}

	public function setConnectTimeout($timeout)
	{
		$this->iConnectTimeoutMS = $timeout;
	}
	public function setSyncCallTimeout($timeout)
	{
		$this->iSyncCallTimeoutMS = $timeout;
	}

	protected function invoke($func, $sReqPayload, &$stRspPacket)
	{
		$stReqPacket = new \mfw\RequestPacket();
		$stReqPacket->bIsOneWay = false;
		$stReqPacket->iRequestId = 1;
		$stReqPacket->sServiceName = $this->sServiceName;
		$stReqPacket->sFuncName = $func;
		$stReqPacket->sReqPayload = $sReqPayload;
		$stReqPacket->iTimeout = $this->iSyncCallTimeoutMS;
		$sReqData = sdpToString($stReqPacket);

		$transport = null;
		$ret = $this->selectTransport($transport);
		if ($ret != 0)
		{
			throw new \Exception("fail to select transport");
		}

		$bIsTimeout = false;
		$ret = $transport->connect($this->iConnectTimeoutMS, $bIsTimeout);
		if ($ret != 0)
		{
			throw new \Exception("fail to connect");
		}

		$sRspData = null;
		$ret = $transport->invoke($sReqData, $sRspData, $this->iSyncCallTimeoutMS);
		if ($ret != 0)
		{
			throw new \Exception("fail to send and recv data");
		}

		$stRspPacket = stringToSdp($sRspData, '\mfw\ResponsePacket');
		if ($stRspPacket == null)
		{
			throw new \Exception("fail to decode response packet");
		}
		if ($stRspPacket->iMfwRet != 0)
		{
			throw new \Exception("mfw framework exception: " . $stRspPacket->iMfwRet);
		}
	}

	private function selectTransport(&$transport)
	{
		$epnum = count($this->vServiceEndpoint);
		if ($epnum == 0)
		{
			return -1;
		}

		foreach ($this->mTransport as $idx => $trans)
		{
			if (!$trans->isError())
			{
				$transport = $trans;
				return 0;
			}
		}
		if (count($this->mTransport) == $epnum)
		{
			$rnd = rand(0, $epnum - 1);
			$transport = $this->mTransport[$rnd];
			return 0;
		}

		$idx = rand(0, $epnum - 1);
		for ($i = 0; $i < $epnum; ++$i)
		{
			if (!array_key_exists($idx, $this->mTransport))
			{
				$transport = new MfwTransport($this->vServiceEndpoint[$idx]);
				$this->mTransport[$idx] = $transport;
				
				$bIsTimeout = false;
				$ret = $transport->connect($this->iConnectTimeoutMS, $bIsTimeout);
				if ($ret == 0)
				{
					return 0;
				}
				else if ($bIsTimeout)
				{
					return -1;
				}
			}
				
			if (++$idx >= $epnum)
			{
				$idx = 0;
			}
		}
		return -1;
	}
}
	
}
