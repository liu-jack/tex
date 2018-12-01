#include "service/AdapterProxy.h"
#include "service/ApplicationConfig.h"
#include "service/ServiceProxy.h"
#include "service/ConnectorImp.h"
#include "service/ReturnCode.h"
#include "util/util_time.h"

namespace mfw
{

AdapterProxy::AdapterProxy(ObjectProxy *pObjectProxy, const CEndpoint &ep) :
    m_pObjectProxy(pObjectProxy),
    m_pTrans(NULL),
    m_ep(ep),
    m_iLastRequestId(0),
    m_iNextStabilityCheckTime(0),
    m_iTotalInvokeNum(0),
    m_iFailInvokeNum(0),
    m_iContinuousFailCheckTime(0),
    m_iContinuousFailNum(0),
    m_bIsActive(true),
    m_iNextRetryTime(0),
    m_bConnTimeout(false),
    m_bConnExc(false)
{
    m_sFullDesc = m_pObjectProxy->getObjectName() + "@" + ep.getDesc();

    if (m_ep.isUdp()) {
        m_pTrans = new UdpTransport(this);
    } else {
        m_pTrans = new TcpTransport(this);
    }
}

AdapterProxy::~AdapterProxy()
{
    if (m_pTrans) {
        delete m_pTrans;
        m_pTrans = NULL;
    }

    CTimeQueue<ReqMessage*, uint64_t>::const_iterator iter = m_pendingRequestQueue.begin();
    while (iter != m_pendingRequestQueue.end()) {
        ReqMessage* msg = (*iter).first;
        if (msg->isOnewayCall()) {
            delete msg;
        } else {
            msg->eMsgStatus = ReqMessage::REQ_TIME;
            if (msg->isSyncCall()) {
                CLockGuard<CNotifier> lock(*msg->pMonitor);
                msg->bMonitorFin = true;
                msg->pMonitor->signal();
            }
        }
        ++iter;
    }

    iter = m_doingRequestQueue.begin();
    while (iter != m_doingRequestQueue.end()) {
        ReqMessage* msg = (*iter).first;
        if (msg->isOnewayCall()) {
            delete msg;
        } else {
            msg->eMsgStatus = ReqMessage::REQ_TIME;
            if (msg->isSyncCall()) {
                CLockGuard<CNotifier> lock(*msg->pMonitor);
                msg->bMonitorFin = true;
                msg->pMonitor->signal();
            }
        }
        ++iter;
    }
}

// 1. 发送对对应Serivce的方法消息
// 2. 缓存消息，建立消息的请求和回应关系
// 3. 由ObjectProxy直接调用
void AdapterProxy::invoke(ReqMessage *msg)
{
    if (m_pendingRequestQueue.size() >= MAX_PENDING_ADAPTER_QUEUE) {
        MFW_ERROR("too many pending request on adapter: " << m_pendingRequestQueue.size() << ", adapter: " << getFullDesc());
        msg->eMsgStatus = ReqMessage::REQ_EXC;
        finishInvoke(msg);
        return;
    }

    if (!msg->bFromRpc) {
        while (true) {
            ++m_iLastRequestId;
            if (m_iLastRequestId == 0) {
                ++ m_iLastRequestId;
            }
            if (m_mRequestId2Msg.find(m_iLastRequestId) == m_mRequestId2Msg.end()) {
                break;
            }
        }
        msg->request.iRequestId = m_iLastRequestId;
        m_pObjectProxy->getClientSideProtocol().requestFunc(msg->request, msg->sReqData);
    } else {
        if (m_pObjectProxy->getClientSideProtocol().requestFunc != NULL) {
            m_pObjectProxy->getClientSideProtocol().requestFunc(msg->request, msg->sReqData);
        }
    }

    if (m_pendingRequestQueue.empty() && m_pTrans->trySendRequest(msg->sReqData.c_str(),msg->sReqData.size())) {
        MFW_DEBUG("push request(send), reqid: " << msg->request.iRequestId << ", adapter: " << getFullDesc());

        if (msg->isOnewayCall()) {
            delete msg;
            return;
        }

        m_doingRequestQueue.add(msg, msg->request.iTimeout + msg->iBeginTime);
    } else {
        MFW_DEBUG("push request(pending), reqid: " << msg->request.iRequestId << ", adapter: " << getFullDesc());

        m_pendingRequestQueue.add(msg, msg->request.iTimeout + msg->iBeginTime);
    }

    if (msg->request.iRequestId != 0 && !msg->isOnewayCall()) {
        m_mRequestId2Msg[msg->request.iRequestId] = msg;
    }
}

// 1. 在消息能够发送后调
// 2. 由ConnectorImp直接调用
void AdapterProxy::doInvoke()
{
    ReqMessage *msg = NULL;
    uint64_t iTime = 0;
    while (m_pendingRequestQueue.peek(msg, iTime)) {
        if (!m_pTrans->trySendRequest(msg->sReqData.c_str(), msg->sReqData.size())) {
            break;
        }

        MFW_DEBUG("send pending request, reqid: " << msg->request.iRequestId << ", adapter: " << getFullDesc());

        m_pendingRequestQueue.del(msg);
        if (msg->isOnewayCall()) {
            delete msg;
        } else {
            m_doingRequestQueue.add(msg, iTime);
        }
    }
}

// 1. 收到Response回应后被调用
// 2. 由ConntorImp直接调用
void AdapterProxy::finishInvoke(ResponsePacket &rsp)
{
    if (rsp.iRequestId == 0) {
        MFW_ERROR("got response with reqid = 0, adapter: " << getFullDesc());
        return;
    }

    map<uint32_t, ReqMessage *>::iterator it = m_mRequestId2Msg.find(rsp.iRequestId);
    if (it == m_mRequestId2Msg.end()) {
        MFW_ERROR("got response but timeout, reqid: " << rsp.iRequestId << ", adapter: " << getFullDesc());
        return;
    }

    MFW_DEBUG("got response, reqid: " << rsp.iRequestId << ", adapter: " << getFullDesc());

    ReqMessage *msg = it->second;
    m_mRequestId2Msg.erase(it);
    m_doingRequestQueue.del(msg);

    msg->eMsgStatus = ReqMessage::REQ_RSP;
    swap(msg->response, rsp);

    finishInvoke(msg);
}

// 1. 处理正常的response
// 2. 处理超时调用
// 3. 处理异常调用 在有太多的未方法消息时触发
void AdapterProxy::finishInvoke(ReqMessage *msg)
{
    if (msg->isOnewayCall()) {
        delete msg;
        return;
    }

    if (msg->eMsgStatus != ReqMessage::REQ_EXC) {
        updateStatus(msg->response.iMfwRet != SDPSERVERSUCCESS);
    }

    if (msg->isSyncCall()) {
        CLockGuard<CNotifier> lock(*msg->pMonitor);
        msg->bMonitorFin = true;
        msg->pMonitor->signal();
    } else if (msg->isAsyncCall()) {
        if (msg->callback->getNetThreadProcess()) {
            try {
                msg->callback->onDispatch(msg);
            } catch (std::exception &e) {
                MFW_ERROR("response dispatch exception: " << e.what());
            }
            delete msg;
        } else {
            m_pObjectProxy->getConnectorImp()->addAsyncRspMsg(msg);
        }
    }
}

void AdapterProxy::updateStatus(bool bFail)
{
    uint32_t iNow = UtilTime::getNow();

    if (!m_bIsActive) {
        if (!bFail) {
            m_iNextStabilityCheckTime = iNow + INVOKE_STABILITY_CHECK_INTERVAL;
            m_iTotalInvokeNum = 1;
            m_iFailInvokeNum = 0;

            m_iContinuousFailCheckTime = 0;
            m_iContinuousFailNum = 0;

            m_bIsActive = true;
            m_bConnTimeout = false;
            m_bConnExc = false;

            MFW_DEBUG("retry ok, adapter: " << getFullDesc());
        } else {
            MFW_DEBUG("retry fail, adapter: " << getFullDesc());
        }
        return;
    }

    ++m_iTotalInvokeNum;

    if (bFail) {
        ++m_iFailInvokeNum;

        if (0 == m_iContinuousFailNum) {
            m_iContinuousFailCheckTime = iNow + INVOKE_CONTINUOUS_FAIL_CHECK_INTERVAL;
        }
        ++m_iContinuousFailNum;

        if (m_iContinuousFailNum >= INVOKE_CONTINUOUS_FAIL_INVOKE_NUM && iNow >= m_iContinuousFailCheckTime) {
            setInactive();
            MFW_ERROR("disable connection(cont fail), continuous fail: " << m_iContinuousFailNum << ", adapter: " << getFullDesc());
            return;
        }
    } else {
        m_iContinuousFailNum = 0;
    }

    if (iNow >= m_iNextStabilityCheckTime) {
        m_iNextStabilityCheckTime = iNow + INVOKE_STABILITY_CHECK_INTERVAL;

        if (bFail && m_iFailInvokeNum >= INVOKE_STABILITY_MIN_FAIL_INVOKE_NUM && m_iFailInvokeNum >= INVOKE_STABILITY_FAIL_RATION100 * m_iTotalInvokeNum / 100) {
            setInactive();
            MFW_ERROR("disable connection(statbility), fail: " << m_iFailInvokeNum << ", total: " << m_iTotalInvokeNum << ", adapter: " << getFullDesc());
        } else {
            m_iTotalInvokeNum = 0;
            m_iFailInvokeNum = 0;
        }
    }
}

// 1. 检测service超时调用
// 2. 由ObjcectProxy直接调用，而ObjectProxy被ConnectorImp直接调用
void AdapterProxy::doTimeout(uint64_t iNowMS)
{
    doTimeout(iNowMS, m_doingRequestQueue);
    doTimeout(iNowMS, m_pendingRequestQueue);
}

void AdapterProxy::doTimeout(uint64_t iNowMS, CTimeQueue<ReqMessage *, uint64_t> &requestQueue)
{
    if (requestQueue.empty()) {
        return;
    }

    ReqMessage *msg = NULL;
    uint64_t iTime = 0;
    while (requestQueue.pop_timeout(iNowMS, msg, iTime)) {
        m_mRequestId2Msg.erase(msg->request.iRequestId);
        msg->eMsgStatus = ReqMessage::REQ_TIME;

        MFW_DEBUG("wait for response timeout, reqid: " << msg->request.iRequestId << ", adapter: " << getFullDesc());

        if (msg->eCallType == ReqMessage::ASYNC_CALL) {
            msg->response.iMfwRet = (m_bConnExc ? SDPPROXYCONNECTERR : SDPASYNCCALLTIMEOUT);
        }

        finishInvoke(msg);
    }
}

// 1. 由EndpointManager在选择AdapterProxy发送消息时调用
bool AdapterProxy::checkActive(bool bForceConnect)
{
    __TRY__
    m_pTrans->checkTimeout();

    if (bForceConnect) {
        if (m_pTrans->isConnecting() || m_pTrans->isConnected()) {
            return true;
        }

        uint32_t iNow = UtilTime::getNow();
        m_iNextRetryTime = iNow + INVOKE_RETRY_INTERVAL;

        if (!m_pTrans->isValid()) {
            m_pTrans->reconnect();
        }
        return (m_pTrans->isConnected() || m_pTrans->isConnecting());
    }

    if (!m_bIsActive) {
        uint32_t iNow = UtilTime::getNow();
        if (iNow < m_iNextRetryTime) {
            return false;
        }

        m_iNextRetryTime = iNow + INVOKE_RETRY_INTERVAL;
    }

    if (!m_pTrans->isValid()) {
        m_pTrans->reconnect();
    }
    return (m_pTrans->isConnected() || m_pTrans->isConnecting());
    __CATCH__
    return false;
}

// 1. 设置Connect是否超时
// 2. 由Transport直接调用
void AdapterProxy::setConnTimeout(bool bConnTimeout)
{
    if (bConnTimeout != m_bConnTimeout) {
        m_bConnTimeout = bConnTimeout;
        if (m_bConnTimeout) {
            setInactive();
        }
    }
}

// 1. 设置Connect是否异常
// 2. 由ConnectorImp直接调用
void AdapterProxy::setConnExc(bool bExc)
{
    if (bExc) {
        m_bConnExc = true;
        setInactive();
    } else {
        m_bConnExc = false;
        m_bIsActive = true;
    }
}

void AdapterProxy::setInactive()
{
    m_bIsActive = false;
    m_iNextRetryTime = UtilTime::getNow() + INVOKE_RETRY_INTERVAL;
    m_pTrans->close();
}

}
