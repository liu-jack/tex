#include "service/Connector.h"

namespace mfw
{

Connector::Connector() {
    m_pConnectorImp = ConnectorImpPtr(new ConnectorImp(this));
}

Connector::~Connector()
{
}

vector<CEndpoint> Connector::getEndpoint(const string &sObjectName)
{
    ServicePrx &pServiceProxy = getServiceProxy(sObjectName);
    return pServiceProxy->mfw_get_endpoint();
}

void Connector::initClientConfig()
{
    ClientConfig::SetOpen = m_commcfg.bIsSetEnabled;
    ClientConfig::SetDivision = m_commcfg.sDivision;
    ClientConfig::ModuleName = m_commcfg.sModuleName.empty() ? "unknown" : m_commcfg.sModuleName;
}

ServicePrx &Connector::getServiceProxy(const string &sObjectName)
{
	CLockGuard<CMutex> lock(m_mutex);

	map<string, ServicePrx>::iterator it = m_mServiceProxy.find(sObjectName);
	if (it != m_mServiceProxy.end())
	{
		return it->second;
	}

    ObjectProxy *pObjectProxy = m_pConnectorImp->getObjectProxy(sObjectName);
    ServicePrx pServiceProxy(new ServiceProxy(pObjectProxy));
    pServiceProxy->mfw_sync_timeout(m_commcfg.iSyncInvokeTimeout);
    pServiceProxy->mfw_async_timeout(m_commcfg.iAsyncInvokeTimeout);
    pServiceProxy->mfw_connect_timeout(m_commcfg.iConnectTimeout);
    return m_mServiceProxy[sObjectName] = pServiceProxy;
}

}
