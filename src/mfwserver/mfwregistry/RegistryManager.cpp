#include "RegistryManager.h"
#include "util/util_log.h"
#include "util/util_network.h"

RegistryManager g_stRegistryManager;

const RegServiceConf *RegServerConf::findService(const string &sService) const
{
    for (unsigned i = 0; i < vService.size(); ++i) {
        if (vService[i].sService == sService) {
            return &vService[i];
        }
    }
    return NULL;
}

void RegistryManager::initialize(const CDBConf &dbconf)
{
    m_mysql.init(dbconf);
    reload();
}

int32_t RegistryManager::reload()
{
    __TRY__

    LOG_DEBUG("reloading db");

    map<string, map<string, vector<string> > > mAppDivisionServerTmp;
    map<string, map<string, vector<string> > > mAppServerDivisionTmp;
    map<string, vector<RegServerConf> > mServerNodesTmp;

    CLockGuard<CMutex> lock(m_mutex);
    CMysqlResultPtr res = m_mysql.query("SELECT * FROM t_server");
    while (res->next()) {
        string sAppName = res->get("app");
        string sServerName = res->get("server");
        string sDivision = res->get("division");
        string sNode = res->get("node");
        uint32_t iStatus = res->get<uint32_t>("status");

        mAppDivisionServerTmp[sAppName][sDivision].push_back(sServerName);
        mAppServerDivisionTmp[sAppName][sServerName].push_back(sDivision);

        RegServerConf stRegServerConf;;
        stRegServerConf.sAppName = sAppName;
        stRegServerConf.sServerName = sServerName;
        stRegServerConf.sDivision = sDivision;
        stRegServerConf.sNode = sNode;
        stRegServerConf.iStatus = iStatus == RegServerStatus_Active ? RegServerStatus_Active : RegServerStatus_Inactive;

        string sLookup = getLookupKey(sAppName, sServerName, sDivision);
        vector<RegServerConf> &vServerConf = mServerNodesTmp[sLookup];
        vServerConf.push_back(stRegServerConf);
    }

    res = m_mysql.query("SELECT * FROM t_service");
    while (res->next()) {
        string sAppName = res->get("app");
        string sServerName = res->get("server");
        string sDivision = res->get("division");
        string sNode = res->get("node");
        string sService = res->get("service");
        string sEndpoint = res->get("endpoint");

        string sLookup = getLookupKey(sAppName, sServerName, sDivision);
        if (mServerNodesTmp.find(sLookup) == mServerNodesTmp.end()) {
            continue;
        }

        vector<RegServerConf> &vServerConf = mServerNodesTmp[sLookup];
        for (unsigned i = 0; i < vServerConf.size(); ++i) {
            RegServerConf &stRegServerConf = vServerConf[i];
            if (stRegServerConf.sNode == sNode) {
                RegServiceConf stRegServiceConf;
                stRegServiceConf.sService = sService;
                stRegServiceConf.sEndpoint = sEndpoint;
                stRegServerConf.vService.push_back(stRegServiceConf);
                break;
            }
        }
    }

    swap(mAppDivisionServerTmp, m_mAppDivisionServer);
    swap(mAppServerDivisionTmp, m_mAppServerDivision);
    swap(mServerNodesTmp, m_mServerNodes);
    m_bDataLoaded = true;

    return 0;
    __CATCH__
    return -1;
}

bool RegistryManager::getServiceEndpoint(const string &sObj, const string &sDivision, vector<string> &vActiveEps, vector<string> &vInactiveEps)
{
    vector<string> vObj = UtilString::splitString(sObj, ".");
    if (vObj.size() != 3) {
        return false;
    }

    const string &sAppName = vObj[0];
    const string &sServerName = vObj[1];
    const string &sService = vObj[2];

    CLockGuard<CMutex> lock(m_mutex);
    if (sDivision.empty()) {
        // return all division
        map<string, map<string, vector<string> > >::const_iterator it1 = m_mAppServerDivision.find(sAppName);
        if (it1 == m_mAppServerDivision.end()) {
            return false;
        }
        map<string, vector<string> >::const_iterator it2 = it1->second.find(sServerName);
        if (it2 == it1->second.end()) {
            return false;
        }

        const vector<string> &vDivision = it2->second;
        for (unsigned i = 0; i < vDivision.size(); ++i) {
            string sLookup = getLookupKey(sAppName, sServerName, vDivision[i]);
            map<string, vector<RegServerConf> >::const_iterator it = m_mServerNodes.find(sLookup);
            if (it != m_mServerNodes.end()) {
                extractServiceEndpoint(it->second, sService, vActiveEps, vInactiveEps);
            }
        }
    } else {
        // return exactly matched division
        string sLookup = getLookupKey(sAppName, sServerName, sDivision);
        map<string, vector<RegServerConf> >::const_iterator it = m_mServerNodes.find(sLookup);
        if (it == m_mServerNodes.end()) {
            // return global server without division
            sLookup = getLookupKey(sAppName, sServerName, "");
            it = m_mServerNodes.find(sLookup);
            if (it == m_mServerNodes.end()) {
                return false;
            }
        }

        extractServiceEndpoint(it->second, sService, vActiveEps, vInactiveEps);
    }
    return true;
}

string RegistryManager::getLookupKey(const string &sAppName, const string &sServerName, const string &sDivision)
{
    return sAppName + "!" + sServerName + "!" + sDivision;
}

void RegistryManager::extractServiceEndpoint(const vector<RegServerConf> &vRegServerConf, const string &sService, vector<string> &vActiveEps, vector<string> &vInactiveEps)
{
    for (unsigned i = 0; i < vRegServerConf.size(); ++i) {
        const RegServerConf &stRegServerConf = vRegServerConf[i];
        const RegServiceConf *pstRegServiceConf = stRegServerConf.findService(sService);
        if (pstRegServiceConf == NULL) {
            continue;
        }

        if (stRegServerConf.isActive()) {
            vActiveEps.push_back(pstRegServiceConf->sEndpoint);
        } else {
            vInactiveEps.push_back(pstRegServiceConf->sEndpoint);
        }
    }
}

bool RegistryManager::addServiceEndpoint(const string &sObj, const string &sDivision, const string &sEp)
{
    vector<string> vObj = UtilString::splitString(sObj, ".");
    if (vObj.size() != 3) {
        return false;
    }

    const string &sAppName = vObj[0];
    const string &sServerName = vObj[1];
    const string &sService = vObj[2];

    CEndpoint stEp;
    if (!stEp.parseNoThrow(sEp)) {
        return false;
    }

    const string &sHost = stEp.getHost();

    string sLookup = getLookupKey(sAppName, sServerName, sDivision);

    bool bAddServer = true;
    bool bAddService = true;

    CLockGuard<CMutex> lock(m_mutex);
    map<string, vector<RegServerConf> >::const_iterator it = m_mServerNodes.find(sLookup);
    if (it != m_mServerNodes.end()) {
        const vector<RegServerConf> &vRegServerConf = it->second;
        for (uint32_t i = 0; i < vRegServerConf.size(); ++i) {
            const RegServerConf &stRegServerConf = vRegServerConf[i];
            if (stRegServerConf.sNode == sHost) {
                bAddServer = false;

                const RegServiceConf *pstRegServiceConf = stRegServerConf.findService(sService);
                if (pstRegServiceConf != NULL) {
                    bAddService = false;
                }
            }
        }
    }

    if (!bAddServer && !bAddService) return false;

    ostringstream os;

    if (bAddServer) {
        os << "INSERT INTO t_server(app,server,division,node) VALUES(";
        os << "\'" << sAppName << "\'" << ",";
        os << "\'" << sServerName << "\'" << ",";
        os << "\'" << sDivision << "\'" << ",";
        os << "\'" << sHost << "\'";
        os <<");";
    }

    if (bAddService) {
        os << "INSERT INTO t_service(app,server,service,division,node,endpoint) VALUES(";
        os << "\'" << sAppName << "\'" << ",";
        os << "\'" << sServerName << "\'" << ",";
        os << "\'" << sService << "\'" << ",";
        os << "\'" << sDivision << "\'" << ",";
        os << "\'" << sHost << "\'" << ",";
        os << "\'" << stEp.getDesc() << "\'";
        os <<");";
    }

    __TRY__

    m_mysql.execute(os.str());
    lock.unlock();

    reload();
    return true;

    __CATCH__
    return false;
}

bool RegistryManager::removeServiceEndpoint(const string &sObj, const string &sDivision, const string &sEp)
{
    vector<string> vObj = UtilString::splitString(sObj, ".");
    if (vObj.size() != 3) {
        return false;
    }

    const string &sAppName = vObj[0];
    const string &sServerName = vObj[1];
    const string &sService = vObj[2];

    CEndpoint stEp;
    if (!stEp.parseNoThrow(sEp)) {
        return false;
    }

    const string &sHost = stEp.getHost();

    string sLookup = getLookupKey(sAppName, sServerName, sDivision);

    bool bRemoveServer = false;
    bool bRemoveService = false;

    CLockGuard<CMutex> lock(m_mutex);
    map<string, vector<RegServerConf> >::const_iterator it = m_mServerNodes.find(sLookup);
    if (it != m_mServerNodes.end()) {
        const vector<RegServerConf> &vRegServerConf = it->second;
        for (uint32_t i = 0; i < vRegServerConf.size(); ++i) {
            const RegServerConf &stRegServerConf = vRegServerConf[i];
            if (stRegServerConf.sNode == sHost) {
                const RegServiceConf *pstRegServiceConf = stRegServerConf.findService(sService);
                if (pstRegServiceConf != NULL) {
                    bRemoveService = true;

                    if (stRegServerConf.vService.size() == 1) {
                        bRemoveServer = true;
                    }
                }
            }
        }
    }

    if (!bRemoveServer && !bRemoveService) return false;

    ostringstream os;

    if (bRemoveServer) {
        os << "DELETE FROM t_server WHERE ";
        os << "app=\'" << sAppName << "\'" << " and ";
        os << "server=\'" << sServerName << "\'" << " and ";
        os << "division=\'" << sDivision << "\'" << " and ";
        os << "node=\'" << sHost << "\';";
    }

    if (bRemoveService) {
        os << "DELETE FROM t_service WHERE ";
        os << "app=\'" << sAppName << "\'" << " and ";
        os << "server=\'" << sServerName << "\'" << " and ";
        os << "service=\'" << sService << "\'" << " and ";
        os << "division=\'" << sDivision << "\'" << " and ";
        os << "node=\'" << sHost << "\'"  << " and ";
        os << "endpoint=\'" << stEp.getDesc() << "\';";
    }

    __TRY__

    m_mysql.execute(os.str());
    lock.unlock();

    reload();
    return true;

    __CATCH__
    return false;
}
