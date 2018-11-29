<?php
namespace mfw
{
require_once 'mfw/MfwClient.php';

class Query extends \mfw\MfwClient
{
	public function getEndpoints($sObj, $sDivision, &$vActiveEps, &$vInactiveEps)
	{
		$_sReqPayload_ = '';
		$_sReqPayload_ .= sdpToString($sObj, '13', 1);
		$_sReqPayload_ .= sdpToString($sDivision, '13', 2);

		$_stRspPacket_ = null;
		$this->invoke('getEndpoints', $_sReqPayload_, $_stRspPacket_);

		list($_ret_, $vActiveEps, $vInactiveEps) = stringToSdp($_stRspPacket_->sRspPayload, array(
			array('7', 0),
			array('vector<13>', 3),
			array('vector<13>', 4),
		));
		return $_ret_;
	}

	public function addEndpoint($sObj, $sDivision, $ep)
	{
		$_sReqPayload_ = '';
		$_sReqPayload_ .= sdpToString($sObj, '13', 1);
		$_sReqPayload_ .= sdpToString($sDivision, '13', 2);
		$_sReqPayload_ .= sdpToString($ep, '13', 3);

		$_stRspPacket_ = null;
		$this->invoke('addEndpoint', $_sReqPayload_, $_stRspPacket_);

		list($_ret_) = stringToSdp($_stRspPacket_->sRspPayload, array(
			array('7', 0),
		));
		return $_ret_;
	}

	public function removeEndpoint($sObj, $sDivision, $ep)
	{
		$_sReqPayload_ = '';
		$_sReqPayload_ .= sdpToString($sObj, '13', 1);
		$_sReqPayload_ .= sdpToString($sDivision, '13', 2);
		$_sReqPayload_ .= sdpToString($ep, '13', 3);

		$_stRspPacket_ = null;
		$this->invoke('removeEndpoint', $_sReqPayload_, $_stRspPacket_);

		list($_ret_) = stringToSdp($_stRspPacket_->sRspPayload, array(
			array('7', 0),
		));
		return $_ret_;
	}
}

}
