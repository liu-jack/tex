#ifndef _MFW_SERVICE_PROXY_H_
#define _MFW_SERVICE_PROXY_H_

#include "service/Message.h"
#include "service/MfwProtocol.h"
#include "util/util_network.h"

namespace mfw
{

class ServiceProxyThreadData
{
public:
    ServiceProxyThreadData() : bSetHash(false), iHashCode(0), bSetTimeout(false), iTimeoutMS(0) {}
    static ServiceProxyThreadData *getData();

public:
    bool        bSetHash;
    uint64_t    iHashCode;
    bool        bSetTimeout;
    uint32_t	iTimeoutMS;
    map<string, string> mContext;
};

class ServiceProxyCallback : public tr1::enable_shared_from_this<ServiceProxyCallback>
{
public:
    ServiceProxyCallback();
    virtual ~ServiceProxyCallback() {}

    void setNetThreadProcess(bool bNetThreadProcess)
    {
        m_bNetThreadProcess = bNetThreadProcess;
    }
    bool getNetThreadProcess()
    {
        return m_bNetThreadProcess;
    }

public:
    virtual void onDispatch(ReqMessage *msg) = 0;

private:
    bool m_bNetThreadProcess;
};

class EndpointThread;
typedef tr1::shared_ptr<EndpointThread> EndpointThreadPtr;

class ServiceProxy
{
public:
    explicit ServiceProxy(ObjectProxy *pObjectProxy);
    virtual ~ServiceProxy() {}

    const string &mfw_name() const;
    void mfw_sync_timeout(uint32_t ms);
    int mfw_sync_timeout() const
    {
        return m_iSyncTimeout;
    }
    void mfw_async_timeout(uint32_t ms);
    int mfw_async_timeout() const
    {
        return m_iAsyncTimeout;
    }
    void mfw_connect_timeout(uint32_t ms);

    void mfw_set_protocol(const ClientSideProtocol &protocol);
    vector<CEndpoint> mfw_get_endpoint();

    void mfw_hash(uint64_t iHashCode);
    void mfw_set_timeout(uint32_t ms);
    void mfw_set_context(const map<string, string> &context);

    void mfw_invoke(const string &sFuncName, const string &sReqPayload, ResponsePacket &rsp);
    void mfw_invoke_async(const string &sFuncName, const string &sReqPayload, const ServiceProxyCallbackPtr &callback);
    void rpc_call(uint32_t iRequestId, const string &sFuncName, const string &sReqPayload, ResponsePacket &rsp);
    void rpc_call_async(uint32_t iRequestId, const string &sFuncName, const string &sReqPayload, const ServiceProxyCallbackPtr &callback);

private:
    void invoke(ReqMessage *msg);

private:
    CMutex m_mutex;
    ObjectProxy *m_pObjectProxy;
    EndpointThreadPtr m_pEndpointQuery;
    uint32_t m_iSyncTimeout;
    uint32_t m_iAsyncTimeout;
};

}
#endif
