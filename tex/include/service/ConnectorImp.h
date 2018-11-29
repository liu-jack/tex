#ifndef _MFW_CONNECTOR_IMP_H_
#define _MFW_CONNECTOR_IMP_H_

#include <map>
#include <vector>
#include "service/Global.h"
#include "util/util_network.h"
#include "util/util_queue.h"

namespace mfw
{

class ConnectorImp
{
public:
    explicit ConnectorImp(Connector *pConnector);
    ~ConnectorImp();

    Connector *getConnector() { return m_pConnector; }
    ObjectProxy *getObjectProxy(const string &sObjectName);

    void addFd(int fd, Transport *pTransport);
    void delFd(int fd);
    void addAsyncRspMsg(ReqMessage *msg);
    void addRequestMsg(ReqMessage *msg);

private:
    void threadClientNet(CThread &thread);
    void threadAsyncRspMsg(CThread &thread);
    void handleInput(Transport *pTransport);
    void handleOutput(Transport *pTransport);
    void doTimeout();

private:
    Connector *m_pConnector;
    CEpoller m_epoller;

    CThreadQueue<ReqMessage *> m_reqMsgQueue;
    CThread m_clientNetThread;

    CThreadQueue<ReqMessage *> m_asyncRspMsgQueue;
    CThreadPool m_asyncRspThreadPool;

    CMutex m_objectProxyMutex;
    map<string, ObjectProxy *> m_mObjectProxy;
    vector<ObjectProxy *> m_vTimeCheckObjectProxy;
    uint64_t m_iNextCheckTime;
};

typedef tr1::shared_ptr<ConnectorImp> ConnectorImpPtr;

}

#endif
