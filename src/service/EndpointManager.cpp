#include "service/EndpointManager.h"
#include "service/ObjectProxy.h"
#include "service/AdapterProxy.h"
#include "service/Application.h"
#include "util/util_string.h"
#include <stdlib.h>

namespace mfw
{

RegistryQueryCallback::RegistryQueryCallback(Connector *pConnector, bool bIsInThread) :
    m_pConnector(pConnector),
    m_bIsInThread(bIsInThread),
    m_bExplicitEndpoint(false),
    m_bValid(false),
    _iRefreshInterval(DEFAULT_ENDPOINT_REFRESH_INTERVAL),
    _iFailInterval(2*1000),
    _iActiveEmptyInterval(10*1000),
    _iMaxFailTimes(3),
    _iManyFailInterval(30*1000),
    _iWaitTime(5*1000),
    _iRequestTimeout(0),
    _bRequest(false),
    _iFailTimes(0),
    _iRefreshTime(0)
{
}

void RegistryQueryCallback::callback_getEndpoints(int32_t ret, const vector<string> &vActiveEps, const vector<string> &vInactiveEps)
{
    doEndpoints(vActiveEps, vInactiveEps, ret);
}

void RegistryQueryCallback::callback_getEndpoints_exception(int32_t ret)
{
    MFW_ERROR("fail to query endpoint: " << ret << ", obj: " << m_sObjectName << ", locator: " << m_sLocator);
    doEndpointsExp();
}

void RegistryQueryCallback::init(const string &sObjectName)
{
    string::size_type pos = sObjectName.find('@');
    if (pos != string::npos) {
        m_sObjectName = sObjectName.substr(0, pos);
        m_bExplicitEndpoint = true;
        m_bValid = true;

        vector<string> vEndpoints = UtilString::splitString(sObjectName.substr(pos + 1), ":");
        for (size_t i = 0; i < vEndpoints.size(); ++i) {
            try {
                CEndpoint ep(vEndpoints[i]);
                m_setActiveEndpoints.insert(ep);
            } catch (...) {
                MFW_ERROR("invalid endpoint:" << vEndpoints[i] << ", obj: " << m_sObjectName);
            }
        }
    } else {
        m_sObjectName = sObjectName;
        m_sDivision = ClientConfig::SetDivision;
        string::size_type pos = sObjectName.find("%");
        if (pos != string::npos) {
            m_sObjectName = sObjectName.substr(0, pos);
            m_sDivision = sObjectName.substr(pos+1);
        }
        m_bExplicitEndpoint = false;
        m_bValid = false;

        m_sLocator = m_pConnector->getConfig().sLocator;
        if (m_sLocator.find('@') == string::npos) {
            throw std::runtime_error("invalid locator:" + m_sLocator);
        }
        m_queryPrx = m_pConnector->stringToProxy<QueryPrx>(m_sLocator);
    }

    if (!m_setActiveEndpoints.empty() || !m_setInactiveEndpoints.empty()) {
        notifyEndpoints(m_setActiveEndpoints, m_setInactiveEndpoints);
    }
}

void RegistryQueryCallback::refreshReg()
{
    if (m_bExplicitEndpoint) {
        return;
    }

    uint64_t iNowMS = UtilTime::getNowMS();
    if (_bRequest && _iRequestTimeout < iNowMS) {
        doEndpointsExp();
    }

    if (_bRequest || iNowMS < _iRefreshTime) {
        return;
    }

    MFW_DEBUG("querying endpoint: " << m_sObjectName);

    _bRequest = true;
    _iRequestTimeout = iNowMS + _iWaitTime;

    try {
        bool bSync = !m_bValid && m_bIsInThread;
        if (bSync) {
            vector<string> vActiveEps;
            vector<string> vInactiveEps;
            int32_t ret = m_queryPrx->getEndpoints(m_sObjectName, m_sDivision, vActiveEps, vInactiveEps);
            doEndpoints(vActiveEps, vInactiveEps, ret);
        } else {
            m_queryPrx->async_getEndpoints(shared_from_this(), m_sObjectName, m_sDivision);
        }
    } catch (std::exception &e) {
        MFW_ERROR("fail to query endpoint, obj: " << m_sObjectName << ", exception: " << e.what());
        doEndpointsExp();
    } catch (...) {
        MFW_ERROR("fail to query endpoint, obj: " << m_sObjectName);
        doEndpointsExp();
    }
}

void RegistryQueryCallback::doEndpoints(const vector<string> &vActiveEps, const vector<string> &vInactiveEps, int32_t ret)
{
    if (ret != 0) {
        doEndpointsExp();
        return;
    }

    _iFailTimes = 0;
    _bRequest = false;

    uint64_t iNowMS = UtilTime::getNowMS();
    if (vActiveEps.empty() && !m_bIsInThread) {
        MFW_ERROR("empty active endpoints, obj: " << m_sObjectName);
        _iRefreshTime = iNowMS + _iActiveEmptyInterval;
        return;
    } else {
        _iRefreshTime = iNowMS + _iRefreshInterval;
    }

    set<CEndpoint> setActiveEps;
    set<CEndpoint> setInactiveEps;
    for (unsigned i = 0; i < vActiveEps.size(); ++i) {
        CEndpoint ep(vActiveEps[i]);
        setActiveEps.insert(ep);
    }
    for (unsigned i = 0; i < vInactiveEps.size(); ++i) {
        CEndpoint ep(vInactiveEps[i]);
        setInactiveEps.insert(ep);
    }

    bool bNeedNotify = false;
    if (setActiveEps != m_setActiveEndpoints) {
        bNeedNotify = true;
        swap(m_setActiveEndpoints, setActiveEps);
    }
    if (setInactiveEps != m_setInactiveEndpoints) {
        bNeedNotify = true;
        swap(m_setInactiveEndpoints, setInactiveEps);
    }

    if (bNeedNotify) {
        notifyEndpoints(m_setActiveEndpoints, m_setInactiveEndpoints);
    }

    if (!m_bValid) {
        m_bValid = true;
        doNotify();
    }
}

void RegistryQueryCallback::doEndpointsExp()
{
    _iFailTimes++;
    _bRequest = false;

    uint64_t iNowMS = UtilTime::getNowMS();
    if (_iFailTimes > _iMaxFailTimes) {
        if (!m_bValid) {
            m_bValid = true;
            doNotify();
        }
        _iRefreshTime = iNowMS + _iManyFailInterval;
    } else {
        _iRefreshTime = iNowMS + _iFailInterval;
    }
}

EndpointManager::EndpointManager(ObjectProxy *pObjectProxy, Connector *pConnector, const string &sObjectName):
    RegistryQueryCallback(pConnector, false),
    m_pObjectProxy(pObjectProxy),
    m_iLastRoundPosition(0)
{
    setNetThreadProcess(true);
    init(sObjectName);
}

EndpointManager::~EndpointManager()
{
    for (map<string, AdapterProxy *>::const_iterator first = m_mAllAdapterProxy.begin(), last = m_mAllAdapterProxy.end(); first != last; ++first) {
        AdapterProxy *pAdapterProxy = first->second;
        delete pAdapterProxy;
    }
}

void EndpointManager::notifyEndpoints(const set<CEndpoint> &setActiveEndpoints, const set<CEndpoint> &/*setInactiveEndpoints*/)
{
    m_vActiveProxys.clear();

    for (set<CEndpoint>::const_iterator first = setActiveEndpoints.begin(), last = setActiveEndpoints.end(); first != last; ++first) {
        const CEndpoint &ep = *first;
        AdapterProxy *pAdapterProxy = NULL;
        map<string, AdapterProxy *>::iterator it = m_mAllAdapterProxy.find(ep.getDesc());
        if (it == m_mAllAdapterProxy.end()) {
            pAdapterProxy = new AdapterProxy(m_pObjectProxy, ep);
            m_mAllAdapterProxy[ep.getDesc()] = pAdapterProxy;
        } else {
            pAdapterProxy = it->second;
        }
        m_vActiveProxys.push_back(pAdapterProxy);
    }
}

void EndpointManager::doNotify()
{
    m_pObjectProxy->doInvoke();
}

bool EndpointManager::selectAdapterProxy(ReqMessage *msg, AdapterProxy *&pAdapterProxy)
{
    pAdapterProxy = NULL;

    refreshReg();
    if (!m_bValid) {
        return true;
    }

    if (msg->bHash) {
        pAdapterProxy = getHashProxy(msg->iHashCode);
    } else {
        pAdapterProxy = getNextValidProxy();
    }
    return false;
}

AdapterProxy *EndpointManager::getNextValidProxy()
{
    if (m_vActiveProxys.empty()) {
        return NULL;
    }

    vector<AdapterProxy *> conn;
    for (unsigned i = 0; i < m_vActiveProxys.size(); ++i) {
        ++m_iLastRoundPosition;
        if (m_iLastRoundPosition >= m_vActiveProxys.size()) {
            m_iLastRoundPosition = 0;
        }

        AdapterProxy *pAdapterProxy = m_vActiveProxys[m_iLastRoundPosition];
        if (pAdapterProxy->checkActive()) {
            return pAdapterProxy;
        }

        if (!pAdapterProxy->isConnTimeout() && !pAdapterProxy->isConnExc()) {
            conn.push_back(pAdapterProxy);
        }
    }

    if (conn.size() > 0) {
        AdapterProxy *pAdapterProxy = conn[rand() % conn.size()];
        if (pAdapterProxy->checkActive(true)) {
            return pAdapterProxy;
        }
    }
    return NULL;
}

AdapterProxy *EndpointManager::getHashProxy(uint64_t iHashCode)
{
    if (m_vActiveProxys.empty()) {
        return NULL;
    }

    AdapterProxy *pAdapterProxy = m_vActiveProxys[iHashCode % m_vActiveProxys.size()];
    if (pAdapterProxy->checkActive()) {
        return pAdapterProxy;
    }

    vector<AdapterProxy *> thisHash = m_vActiveProxys;
    vector<AdapterProxy *> conn;
    while (!thisHash.empty()) {
        uint32_t hash = iHashCode % thisHash.size();
        AdapterProxy *pAdapterProxy = thisHash[hash];

        if (pAdapterProxy->checkActive()) {
            return pAdapterProxy;
        }
        if (!pAdapterProxy->isConnTimeout() && !pAdapterProxy->isConnExc()) {
            conn.push_back(pAdapterProxy);
        }
        thisHash.erase(thisHash.begin() + hash);
    }

    if (conn.size() > 0) {
        AdapterProxy *pAdapterProxy = conn[iHashCode % conn.size()];
        if (pAdapterProxy->checkActive(true)) {
            return pAdapterProxy;
        }
    }
    return NULL;
}

EndpointThread::EndpointThread(Connector *pConnector, const string &sObjectName) :
    RegistryQueryCallback(pConnector, true)
{
    init(sObjectName);
}

void EndpointThread::getEndpoint(vector<CEndpoint> &vActiveEndpoint, vector<CEndpoint> &vInactiveEndpoint)
{
    refreshReg();
    vActiveEndpoint = m_vActiveEndpoint;
    vInactiveEndpoint = m_vInactiveEndpoint;
}

void EndpointThread::notifyEndpoints(const set<CEndpoint> &setActiveEndpoints, const set<CEndpoint> &setInactiveEndpoints)
{
    m_vActiveEndpoint.assign(setActiveEndpoints.begin(), setActiveEndpoints.end());
    m_vInactiveEndpoint.assign(setInactiveEndpoints.begin(), setInactiveEndpoints.end());
}

}

