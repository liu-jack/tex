#include "service/ConnectorImp.h"
#include "service/Connector.h"
#include "service/ObjectProxy.h"
#include "service/AdapterProxy.h"
#include "service/Transport.h"
#include "util/util_time.h"

namespace mfw
{

ConnectorImp::ConnectorImp(Connector *pConnector)
    : m_clientNetThread("client-net"),
      m_asyncRspThreadPool("async-rsp"),
      m_iNextCheckTime(0)
{
    m_pConnector = pConnector;
    m_epoller.setEnableNotify(0);

    m_clientNetThread.setRoutine(tr1::bind(&ConnectorImp::threadClientNet, this, tr1::placeholders::_1));
    m_clientNetThread.setTerminate(tr1::bind(&CEpoller::notify, &m_epoller, 0));
    m_clientNetThread.start();

    m_asyncRspThreadPool.setRoutine(tr1::bind(&ConnectorImp::threadAsyncRspMsg, this, tr1::placeholders::_1));
    m_asyncRspThreadPool.setTerminate(tr1::bind(&CThreadQueue<ReqMessage *>::wakeup, &m_asyncRspMsgQueue));
    m_asyncRspThreadPool.setThreadNum(m_pConnector->getConfig().iAsyncThreadNum);
    m_asyncRspThreadPool.start();

    m_vTimeCheckObjectProxy.reserve(MAX_OBJECT_PROXY_NUM);
}

ConnectorImp::~ConnectorImp()
{
    m_clientNetThread.terminate();
    m_asyncRspThreadPool.terminate();

    m_clientNetThread.join();
    m_asyncRspThreadPool.join();

    for (unsigned i = 0; i < m_vTimeCheckObjectProxy.size(); ++i) {
        delete m_vTimeCheckObjectProxy[i];
        m_vTimeCheckObjectProxy[i] = NULL;
    }
}

ObjectProxy *ConnectorImp::getObjectProxy(const string &sObjectName)
{
    CLockGuard<CMutex> lock(m_objectProxyMutex);
    map<string, ObjectProxy*>::iterator it = m_mObjectProxy.find(sObjectName);
    if (it != m_mObjectProxy.end()) {
        return it->second;
    }

    if (m_vTimeCheckObjectProxy.size() == m_vTimeCheckObjectProxy.capacity()) {
        throw std::runtime_error("too many object proxy");
    }

    ObjectProxy *pObjectProxy = new ObjectProxy(this, sObjectName);
    m_mObjectProxy[sObjectName] = pObjectProxy;
    m_vTimeCheckObjectProxy.push_back(pObjectProxy);
    return pObjectProxy;
}

void ConnectorImp::addFd(int fd, Transport *pTransport)
{
    m_epoller.add(fd, (uint64_t)pTransport, EPOLLIN | EPOLLOUT);
}

void ConnectorImp::delFd(int fd)
{
    m_epoller.del(fd);
}

void ConnectorImp::handleInput(Transport *pTransport)
{
    list<ResponsePacket> done;
    if (pTransport->doResponse(done) > 0) {
        list<ResponsePacket>::iterator it = done.begin();
        for (; it != done.end(); ++it) {
            pTransport->getAdapterProxy()->finishInvoke(*it);
        }
    }
}

void ConnectorImp::handleOutput(Transport *pTransport)
{
    pTransport->doRequest();
}

void ConnectorImp::doTimeout()
{
    uint64_t iNowMS = UtilTime::getNowMS();
    if(m_iNextCheckTime > iNowMS) {
        return;
    }

    m_iNextCheckTime = iNowMS + INVOKE_TIMEOUT_CHECK_INTERVAL;
    for (unsigned i = 0; i < m_vTimeCheckObjectProxy.size(); ++i) {
        m_vTimeCheckObjectProxy[i]->doTimeout(iNowMS);
    }
}

void ConnectorImp::addAsyncRspMsg(ReqMessage *msg)
{
    m_asyncRspMsgQueue.enqueue(msg);
}

void ConnectorImp::addRequestMsg(ReqMessage *msg)
{
    m_reqMsgQueue.enqueue(msg);
    m_epoller.notify(0);
}

void ConnectorImp::threadClientNet(CThread &thread)
{
    while (!thread.isTerminate()) {
        __TRY__

        int num = m_epoller.wait(INVOKE_TIMEOUT_CHECK_INTERVAL);
        for (int i = 0; i < num; ++i) {
            epoll_event &ev = m_epoller.get(i);
            if (ev.data.u64 == 0) {
                ReqMessage *msg = NULL;
                while (m_reqMsgQueue.dequeue(msg, 0)) {
                    __TRY__
                    msg->pObjectProxy->invoke(msg);
                    __CATCH__
                }
            } else {
                Transport *pTransport = (Transport *)ev.data.u64;
                if (pTransport->isConnecting()) {
                    if (ev.events & EPOLLERR) {
                        int iCode = pTransport->getSocket().getsockerror();
                        MFW_ERROR("connect to " << pTransport->getAdapterProxy()->getFullDesc() << " error: " << strerror(iCode));

                        pTransport->getAdapterProxy()->setConnExc(true);
                        pTransport->close();
                        continue;
                    }
                    if (ev.events & (EPOLLIN | EPOLLOUT)) {
                        pTransport->setConnected();
                        MFW_DEBUG("connected to " << pTransport->getAdapterProxy()->getFullDesc() << ", fd: " << pTransport->getSocket().getfd());
                    }
                }

                if (ev.events & EPOLLIN) {
                    __TRY__
                    handleInput(pTransport);
                    __CATCH__
                }
                if (ev.events & EPOLLOUT) {
                    __TRY__
                    handleOutput(pTransport);
                    __CATCH__
                }
                if (ev.events & EPOLLERR) {
                    __TRY__
                    pTransport->close();
                    __CATCH__
                }
            }
        }
        doTimeout();
        __CATCH__
    }
}

void ConnectorImp::threadAsyncRspMsg(CThread &thread)
{
    while (!thread.isTerminate()) {
        ReqMessage *msg = NULL;
        if (m_asyncRspMsgQueue.dequeue(msg)) {
            __TRY__
            msg->callback->onDispatch(msg);
            __CATCH__

            delete msg;
        }
    }
}

}
