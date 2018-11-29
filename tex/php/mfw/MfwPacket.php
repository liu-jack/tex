<?php
namespace mfw
{
class RequestPacket
{
	public $bIsOneWay;
	public $iRequestId;
	public $sServiceName;
	public $sFuncName;
	public $sReqPayload;
	public $iTimeout;
	public $context;

	function __construct()
	{
		$this->bIsOneWay = false;
		$this->iRequestId = 0;
		$this->sServiceName = '';
		$this->sFuncName = '';
		$this->sReqPayload = '';
		$this->iTimeout = 0;
		$this->context = array();
	}
	public static $__Definition = array(
		'bIsOneWay', 'iRequestId', 'sServiceName', 'sFuncName', 'sReqPayload', 'iTimeout', 'context', 
		'bIsOneWay' => array(0, 0, '1', false),
		'iRequestId' => array(1, 0, '8', 0),
		'sServiceName' => array(2, 0, '13', ''),
		'sFuncName' => array(3, 0, '13', ''),
		'sReqPayload' => array(4, 0, '13', ''),
		'iTimeout' => array(5, 0, '8', 0),
		'context' => array(6, 0, 'map<13, 13>', null),
	);
}

class ResponsePacket
{
	public $iMfwRet;
	public $iRequestId;
	public $sRspPayload;
	public $context;

	function __construct()
	{
		$this->iMfwRet = 0;
		$this->iRequestId = 0;
		$this->sRspPayload = '';
		$this->context = array();
	}
	public static $__Definition = array(
		'iMfwRet', 'iRequestId', 'sRspPayload', 'context', 
		'iMfwRet' => array(0, 0, '7', 0),
		'iRequestId' => array(1, 0, '8', 0),
		'sRspPayload' => array(2, 0, '13', ''),
		'context' => array(3, 0, 'map<13, 13>', null),
	);
}

}
