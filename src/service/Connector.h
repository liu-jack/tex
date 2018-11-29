#ifndef _MFW_CONNECTOR_H_
#define _MFW_CONNECTOR_H_

#include "service/ApplicationConfig.h"
#include "service/ServiceProxy.h"
#include "service/ConnectorImp.h"
#include "util/util_thread.h"
#include "util/util_network.h"

namespace mfw
{

class Connector
{
public:
    Connector();
    ~Connector();

    void setConfig(const ConnectorConfig &cfg) { m_commcfg = cfg; initClientConfig(); }
    const ConnectorConfig &getConfig() const { return m_commcfg; }

    template<class T> T stringToProxy(const string &sObjectName)
    {
    	ServicePrx &pServiceProxy = getServiceProxy(sObjectName);
        return tr1::static_pointer_cast<typename T::element_type>(pServiceProxy);
    }

    vector<CEndpoint> getEndpoint(const string &sObjectName);

protected:
    void initialize();
    void initClientConfig();

    ServicePrx &getServiceProxy(const string &sObjectName);

protected:
    CMutex m_mutex;
    ConnectorConfig m_commcfg;
    ConnectorImpPtr m_pConnectorImp;
    map<string, ServicePrx> m_mServiceProxy;
};

}
#endif
