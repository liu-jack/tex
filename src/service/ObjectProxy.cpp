#include "service/ObjectProxy.h"
#include "service/ApplicationConfig.h"
#include "service/AdapterProxy.h"
#include "service/EndpointManager.h"
#include "service/ConnectorImp.h"
#include "service/ReturnCode.h"

namespace mfw
{

ObjectProxy::ObjectProxy(ConnectorImp * pConnectorImp, const string &sObjectName) :
	m_pConnectorImp(pConnectorImp),
	m_iConnectTimeout(CONNECT_DEFAULT_TIMEOUT),
	m_pServiceProxy(NULL)
{
	m_sFullObjectName = sObjectName;

    string::size_type pos = m_sFullObjectName.find_first_of('@');
    if (pos != string::npos)
    {
        m_sObjectName = m_sFullObjectName.substr(0,pos);
    }
    else
    {
        m_sObjectName = m_sFullObjectName;
    }

	m_sDivision = ClientConfig::SetDivision;
	pos = m_sObjectName.find_first_of('%');
	if (pos != string::npos)
	{
		m_sDivision = m_sObjectName.substr(pos+1);
		m_sObjectName = m_sObjectName.substr(0, pos);
	}

    m_pEndpointManger = EndpointManagerPtr(new EndpointManager(this, m_pConnectorImp->getConnector(),m_sFullObjectName));

}

ObjectProxy::~ObjectProxy()
{
	CTimeQueue<ReqMessage*, uint64_t>::const_iterator iter = m_delayRequestQueue.begin();
	while (iter != m_delayRequestQueue.end())
	{
		ReqMessage* msg = (*iter).first;
		if (msg->isOnewayCall())
    	{
			delete msg;
    	}
		else
		{
    		msg->eMsgStatus = ReqMessage::REQ_TIME;
    		if (msg->isSyncCall())
    		{
    			CLockGuard<CNotifier> lock(*msg->pMonitor);
    			msg->bMonitorFin = true;
    			msg->pMonitor->signal();
    		}
		}
		++iter;
	}
}

// 1. 缓存应用层的方法调用(消息)
// 2. 选择合适的AdapterProxy来发送消息
// 3. 由ConnectorImp直接调用
void ObjectProxy::invoke(ReqMessage *msg)
{
    AdapterProxy *pAdapterProxy = NULL;
    bool bFirst = m_pEndpointManger->selectAdapterProxy(msg, pAdapterProxy);
    if (bFirst)
    {
    	m_delayRequestQueue.add(msg, msg->request.iTimeout + msg->iBeginTime);
    	MFW_DEBUG("delay request, query registry first, obj: " << m_sObjectName);
        return;
    }

    if (!pAdapterProxy)
    {
    	MFW_ERROR("no adapter for obj: " << m_sObjectName);
        msg->response.iMfwRet = SDPADAPTERNULL;
        doInvokeException(msg);
        return;
    }

    msg->pAdapterProxy = pAdapterProxy;
    pAdapterProxy->invoke(msg);
}

// 1. 延迟调用ObjectProxy保存的消息
// 2. 当有可用的AdapterProxy可用时被调用
// 3. 有EndpointManager直接调用
void ObjectProxy::doInvoke()
{
	ReqMessage *msg = NULL;
	uint64_t iTime = 0;
    while (m_delayRequestQueue.pop(msg, iTime))
    {
    	MFW_DEBUG("process delayed request, obj: " << m_sObjectName);

        AdapterProxy *pAdapterProxy = NULL;
        m_pEndpointManger->selectAdapterProxy(msg, pAdapterProxy);
        if (!pAdapterProxy)
        {
        	MFW_ERROR("no adapter for obj: " << m_sObjectName);
            msg->response.iMfwRet = SDPADAPTERNULL;
            doInvokeException(msg);
            continue;
        }

        msg->pAdapterProxy = pAdapterProxy;
        pAdapterProxy->invoke(msg);
    }
}

// 处理调用方法产生的异常
// 1. 超时异常
// 2. 未找到AdapterProxy异常 由ObjectProxy在找AdapterProxy时直接调用
void ObjectProxy::doInvokeException(ReqMessage *msg)
{
    if (msg->isOnewayCall())
    {
        delete msg;
        return;
    }

    msg->eMsgStatus = ReqMessage::REQ_EXC;
    if (msg->isSyncCall())
    {
    	CLockGuard<CNotifier> lock(*msg->pMonitor);
    	msg->bMonitorFin = true;
    	msg->pMonitor->signal();
        return;
    }

    if (msg->callback)
    {
        if (msg->callback->getNetThreadProcess())
        {
            try
            {
                msg->callback->onDispatch(msg);
            }
            catch (std::exception &e)
            {
            	MFW_ERROR("response dispatch exception: " << e.what());
            }
            delete msg;
        }
        else
        {
            m_pConnectorImp->addAsyncRspMsg(msg);
        }
    }
}

// 处理每个方法调用超时
// 由ConnectorImp直接调用
void ObjectProxy::doTimeout(uint64_t iNowMS)
{
	if (!m_delayRequestQueue.empty())
	{
		ReqMessage *msg = NULL;
		uint64_t iTime = 0;
		while (m_delayRequestQueue.pop_timeout(iNowMS, msg, iTime))
		{
			MFW_ERROR("delay request timeout, obj: " << m_sObjectName);
	        msg->response.iMfwRet = SDPINVOKETIMEOUT;
	        doInvokeException(msg);
		}
	}

    const map<string, AdapterProxy *> &mAllAdapterProxy = m_pEndpointManger->getAllAdapters();
    for (map<string, AdapterProxy *>::const_iterator first = mAllAdapterProxy.begin(), last = mAllAdapterProxy.end(); first != last; ++first)
    {
    	AdapterProxy *pAdapterProxy = first->second;
        pAdapterProxy->doTimeout(iNowMS);
    }
}

}
