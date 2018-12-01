#ifndef _MFW_ENDPOINT_MANAGER_H_
#define _MFW_ENDPOINT_MANAGER_H_

#include "util/util_network.h"
#include "registry/Query.h"

namespace mfw
{

class RegistryQueryCallback : public QueryPrxCallback
{
public:
    RegistryQueryCallback(Connector *pConnector, bool bIsInThread);

    virtual void callback_getEndpoints(int32_t ret, const vector<string> &vActiveEps,  const vector<string> &vInactiveEps);
    virtual void callback_getEndpoints_exception(int32_t ret);

protected:
    void init(const string &sObjectName);
    void refreshReg();

    virtual void notifyEndpoints(const set<CEndpoint> &setActiveEndpoints, const set<CEndpoint> &setInactiveEndpoints) = 0;
    virtual void doNotify() = 0;

private:
    void doEndpoints(const vector<string> &vActiveEps, const vector<string> &vInactiveEps, int32_t ret);
    void doEndpointsExp();

protected:
    Connector *m_pConnector;
    bool m_bIsInThread;

    string m_sObjectName;
    string m_sDivision;
    bool m_bExplicitEndpoint;
    string m_sLocator;
    QueryPrx m_queryPrx;

    bool m_bValid;
    set<CEndpoint> m_setActiveEndpoints;
    set<CEndpoint> m_setInactiveEndpoints;

private:
    int _iRefreshInterval;
    int _iFailInterval;
    int _iActiveEmptyInterval;
    int _iMaxFailTimes;
    int _iManyFailInterval;
    int _iWaitTime;

    uint64_t _iRequestTimeout;
    bool _bRequest;
    int _iFailTimes;
    uint64_t _iRefreshTime;
};

class EndpointManager : public RegistryQueryCallback
{
public:
    EndpointManager(ObjectProxy *pObjectProxy, Connector *pConnector, const string &sObjectName);
    virtual ~EndpointManager();

    bool selectAdapterProxy(ReqMessage *msg, AdapterProxy *&pAdapterProxy);
    const map<string, AdapterProxy *> &getAllAdapters() const
    {
        return m_mAllAdapterProxy;
    }

protected:
    void notifyEndpoints(const set<CEndpoint> &setActiveEndpoints, const set<CEndpoint> &setInactiveEndpoints);
    void doNotify();

private:
    AdapterProxy *getNextValidProxy();
    AdapterProxy *getHashProxy(uint64_t iHashCode);

private:
    ObjectProxy *m_pObjectProxy;
    uint32_t m_iLastRoundPosition;
    vector<AdapterProxy *> m_vActiveProxys;
    map<string, AdapterProxy *> m_mAllAdapterProxy;
};

class EndpointThread : public RegistryQueryCallback
{
public:
    EndpointThread(Connector *pConnector, const string &sObjectName);

    void getEndpoint(vector<CEndpoint> &vActiveEndpoint, vector<CEndpoint> &vInactiveEndpoint);

protected:
    void notifyEndpoints(const set<CEndpoint> &setActiveEndpoints, const set<CEndpoint> &setInactiveEndpoints);
    void doNotify() {}

private:
    vector<CEndpoint> m_vActiveEndpoint;
    vector<CEndpoint> m_vInactiveEndpoint;
};

}
#endif
