#include "service/SdpCurrent.h"
#include "service/ServiceHandle.h"
#include "service/ReturnCode.h"
#include "service/Application.h"
#include <cerrno>

namespace mfw
{

SdpCurrent::SdpCurrent(ServiceHandle *pServiceHandle) :
	m_pServiceHandle(pServiceHandle),
	m_pBindAdapter(NULL),
	m_iBegintime(0),
	m_iPort(0),
	m_iUid(0),
	m_bResponse(true)
{
}

void SdpCurrent::initialize(const tagRecvData &stRecvData)
{
	m_sIp = stRecvData.ip;
    m_iPort = stRecvData.port;
    m_iUid = stRecvData.uid;
    m_pBindAdapter = stRecvData.adapter;
	m_iBegintime = stRecvData.recvTimeStamp;

    if (m_pBindAdapter->isMfwProtocol())
    {
    	stringToSdp(stRecvData.buffer, m_request);
    }
    else
    {
    	m_request.sServiceName = m_pBindAdapter->getServiceName();
        m_request.sReqPayload = stRecvData.buffer;
    }
}

void SdpCurrent::initializeClose(const tagRecvData &stRecvData)
{
	m_sIp = stRecvData.ip;
    m_iPort = stRecvData.port;
    m_iUid = stRecvData.uid;
    m_pBindAdapter = stRecvData.adapter;
    m_iBegintime = UtilTime::getNowMS();

	m_request.sServiceName = m_pBindAdapter->getServiceName();
}

void SdpCurrent::sendNoMfwResponse(const string &sData)
{
    m_pServiceHandle->sendResponse(m_iUid, sData, m_sIp, m_iPort);
}

void SdpCurrent::sendMfwResponse(int32_t iMfwRet, const string &sRspPayload)
{
    if (isOneWay())
    {
        return;
    }

    MFW_DEBUG("mfw response, peer: " << m_sIp << ":" << m_iPort << ", obj: " << getServiceName() << ", func: " << getFuncName() << ", reqid: " << getRequestId());

	ResponsePacket response;
	response.iMfwRet = iMfwRet;
	response.iRequestId = m_request.iRequestId;
	response.sRspPayload = sRspPayload;
	response.context = m_responseContext;

	SdpPacker packer;
	packer.pack(response);

	string sData;
    uint32_t iHeaderLen = htonl(sizeof(uint32_t) + packer.getData().size());
    sData.append((const char*)&iHeaderLen, sizeof(uint32_t));
    sData.append(packer.getData());
    m_pServiceHandle->sendResponse(m_iUid, sData, m_sIp, m_iPort);
}

void SdpCurrent::close()
{
    if (m_pServiceHandle)
    {
        m_pServiceHandle->closeConnection(m_iUid);
    }
}

}
