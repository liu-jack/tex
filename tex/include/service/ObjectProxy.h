#ifndef _MFW_OBJECT_PROXY_H_
#define _MFW_OBJECT_PROXY_H_

#include "service/Global.h"
#include "service/MfwProtocol.h"
#include "util/util_queue.h"

namespace mfw
{

class EndpointManager;
typedef tr1::shared_ptr<EndpointManager> EndpointManagerPtr;

class ObjectProxy
{
public:
    ObjectProxy(ConnectorImp *pConnectorImp, const string &sObjectName);
    ~ObjectProxy();

    ConnectorImp *getConnectorImp()
    {
        return m_pConnectorImp;
    }
    const string &getObjectName() const
    {
        return m_sObjectName;
    }
    const string &getDivision() const
    {
        return m_sDivision;
    }
    const string &getFullObjectName() const
    {
        return m_sFullObjectName;
    }
    void setClientSideProtocol(const ClientSideProtocol &protocol)
    {
        m_clientSideProtocol = protocol;
    }
    ClientSideProtocol &getClientSideProtocol()
    {
        return m_clientSideProtocol;
    }
    uint32_t getConnectTimeout()
    {
        return m_iConnectTimeout;
    }
    void setConnectTimeout(uint32_t iConnectTimeout)
    {
        m_iConnectTimeout = iConnectTimeout;
    }
    ServiceProxy *getServiceProxy()
    {
        return m_pServiceProxy;
    }
    void setServiceProxy(ServiceProxy *pServiceProxy)
    {
        m_pServiceProxy = pServiceProxy;
    }

    void invoke(ReqMessage *msg);
    void doInvoke();
    void doTimeout(uint64_t iNowMS);

private:
    void doInvokeException(ReqMessage *msg);

private:
    ConnectorImp *m_pConnectorImp;
    string m_sObjectName;
    string m_sDivision;
    string m_sFullObjectName;
    ClientSideProtocol m_clientSideProtocol;
    uint32_t m_iConnectTimeout;
    ServiceProxy *m_pServiceProxy;

    EndpointManagerPtr m_pEndpointManger;
    CTimeQueue<ReqMessage *, uint64_t> m_delayRequestQueue;
};

}
#endif
