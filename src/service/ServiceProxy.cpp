#include "service/ServiceProxy.h"
#include "service/ReturnCode.h"
#include "service/ApplicationConfig.h"
#include "service/EndpointManager.h"
#include "service/ConnectorImp.h"
#include "service/ObjectProxy.h"
#include "service/AdapterProxy.h"
#include "util/util_singleton.h"

namespace mfw
{

ServiceProxyThreadData * ServiceProxyThreadData::getData()
{
	return ThreadSingleton<ServiceProxyThreadData>::get();
}

ServiceProxyCallback::ServiceProxyCallback()
{
    m_bNetThreadProcess = false;
}

ServiceProxy::ServiceProxy(ObjectProxy *pObjectProxy) :
	m_pObjectProxy(pObjectProxy),
	m_iSyncTimeout(SYNC_CALL_DEFAULT_TIMEOUT),
	m_iAsyncTimeout(ASYNC_CALL_DEFAULT_TIMEOUT)
{
	m_pObjectProxy->setServiceProxy(this);
}

const string &ServiceProxy::mfw_name() const
{
	return m_pObjectProxy->getObjectName();
}

void ServiceProxy::mfw_sync_timeout(uint32_t ms)
{
	m_iSyncTimeout = std::max(ms, (uint32_t)SYNC_CALL_MIN_TIMEOUT);
}

void ServiceProxy::mfw_async_timeout(uint32_t ms)
{
	m_iAsyncTimeout = std::max(ms, (uint32_t)ASYNC_CALL_MIN_TIMEOUT);
}

void ServiceProxy::mfw_connect_timeout(uint32_t ms)
{
	ms = std::max(ms, (uint32_t)CONNECT_MIN_TIMEOUT);
	ms = std::min(ms, (uint32_t)CONNECT_MAX_TIMEOUT);
    m_pObjectProxy->setConnectTimeout(ms);
}

void ServiceProxy::mfw_set_protocol(const ClientSideProtocol& protocol)
{
	CLockGuard<CMutex> lock(m_mutex);
    m_pObjectProxy->setClientSideProtocol(protocol);
}

vector<CEndpoint> ServiceProxy::mfw_get_endpoint()
{
	CLockGuard<CMutex> lock(m_mutex);
	if (!m_pEndpointQuery)
	{
		m_pEndpointQuery = EndpointThreadPtr(new EndpointThread(m_pObjectProxy->getConnectorImp()->getConnector(), m_pObjectProxy->getFullObjectName()));
	}

    vector<CEndpoint> vActiveEndpoint;
    vector<CEndpoint> vInactiveEndpoint;
    m_pEndpointQuery->getEndpoint(vActiveEndpoint, vInactiveEndpoint);
    return vActiveEndpoint;
}

void ServiceProxy::mfw_hash(uint64_t iHashCode)
{
    ServiceProxyThreadData *pSptd = ServiceProxyThreadData::getData();
    pSptd->bSetHash = true;
    pSptd->iHashCode = iHashCode;
}

void ServiceProxy::mfw_set_timeout(uint32_t ms)
{
	ServiceProxyThreadData *pSptd = ServiceProxyThreadData::getData();
    pSptd->bSetTimeout = true;
    pSptd->iTimeoutMS = ms;
}

void ServiceProxy::mfw_set_context(const map<string, string> &context)
{
	ServiceProxyThreadData *pSptd = ServiceProxyThreadData::getData();
	pSptd->mContext = context;
}

void ServiceProxy::mfw_invoke(const string &sFuncName,const string &sReqPayload, ResponsePacket &rsp)
{
    ReqMessage *msg = new ReqMessage();
    msg->eCallType = ReqMessage::SYNC_CALL;

    msg->request.bIsOneWay = false;
    msg->request.sServiceName = m_pObjectProxy->getObjectName();
    msg->request.sFuncName = sFuncName;
    msg->request.sReqPayload = sReqPayload;
    msg->request.iTimeout = m_iSyncTimeout;

    invoke(msg);
    swap(rsp, msg->response);
	delete msg;
}

void ServiceProxy::mfw_invoke_async(const string &sFuncName, const string &sReqPayload, const ServiceProxyCallbackPtr &callback)
{
    ReqMessage *msg = new ReqMessage();
    msg->eCallType = callback ? ReqMessage::ASYNC_CALL : ReqMessage::ONE_WAY;
    msg->callback = callback;

    msg->request.bIsOneWay = callback ? false : true;
    msg->request.sServiceName = m_pObjectProxy->getObjectName();
    msg->request.sFuncName = sFuncName;
    msg->request.sReqPayload = sReqPayload;
    msg->request.iTimeout = m_iAsyncTimeout;

    invoke(msg);
}

void ServiceProxy::rpc_call(uint32_t iRequestId, const string &sFuncName, const string &sReqPayload, ResponsePacket &rsp)
{
    ReqMessage *msg = new ReqMessage();
    msg->eCallType = ReqMessage::SYNC_CALL;
    msg->bFromRpc = true;

    msg->request.iRequestId = iRequestId;
    msg->request.sFuncName = sFuncName;
    if (m_pObjectProxy->getClientSideProtocol().requestFunc != NULL)
    {
    	msg->request.sReqPayload = sReqPayload;
    }
    else
    {
    	msg->sReqData = sReqPayload;
    }

    invoke(msg);
	swap(rsp, msg->response);
	delete msg;
}

void ServiceProxy::rpc_call_async(uint32_t iRequestId, const string &sFuncName, const string &sReqPayload, const ServiceProxyCallbackPtr &callback)
{
    ReqMessage *msg = new ReqMessage();
    msg->eCallType = callback ? ReqMessage::ASYNC_CALL : ReqMessage::ONE_WAY;
    msg->bFromRpc = true;
    msg->callback = callback;

    msg->request.iRequestId = iRequestId;
    msg->request.sFuncName = sFuncName;
    if (m_pObjectProxy->getClientSideProtocol().requestFunc != NULL)
    {
    	msg->request.sReqPayload = sReqPayload;
    }
    else
    {
    	msg->sReqData = sReqPayload;
    }

    invoke(msg);
}

void ServiceProxy::invoke(ReqMessage *msg)
{
	msg->response.iMfwRet = SDPSERVERUNKNOWNERR;

    ServiceProxyThreadData *pSptd = ServiceProxyThreadData::getData();
    msg->bHash = pSptd->bSetHash;
    msg->iHashCode = pSptd->iHashCode;
    pSptd->bSetHash = false;

	msg->request.iTimeout = msg->isSyncCall() ? m_iSyncTimeout : m_iAsyncTimeout;
	if (pSptd->bSetTimeout)
	{
		msg->request.iTimeout = pSptd->iTimeoutMS;
		pSptd->bSetTimeout = false;
	}

	if (!pSptd->mContext.empty())
	{
		msg->request.context = pSptd->mContext;
		pSptd->mContext.clear();
	}

    msg->iBeginTime = UtilTime::getNowMS();
    msg->pObjectProxy = m_pObjectProxy;

    if (msg->isSyncCall())
    {
        msg->bMonitorFin = false;
        msg->pMonitor = new CNotifier();
    }

    bool bIsSyncCall = msg->isSyncCall();
    m_pObjectProxy->getConnectorImp()->addRequestMsg(msg);
    if (!bIsSyncCall)
    {
    	return;
    }

	if (!msg->bMonitorFin)
	{
		CLockGuard<CNotifier> lock(*msg->pMonitor);
		if (!msg->bMonitorFin)
		{
			msg->pMonitor->wait();
		}
	}

	if (msg->eMsgStatus == ReqMessage::REQ_RSP && msg->response.iMfwRet == SDPSERVERSUCCESS)
	{
		return;
	}

	ostringstream os;
	if (msg->eMsgStatus == ReqMessage::REQ_TIME)
	{
		os << "invoke timeout: " << msg->request.iTimeout;
	}
	else
	{
		os << "invoke exception: " << msg->response.iMfwRet;
	}
	os << ", service: " << msg->pObjectProxy->getObjectName();
	os << ", func: " << msg->request.sFuncName;
	if (msg->pAdapterProxy)
	{
		os << ", adapter: " << msg->pAdapterProxy->getEndpoint().getDesc();
	}
	os << ", reqid: " << msg->request.iRequestId;

	delete msg;
	throw std::runtime_error(os.str());
}

}
