#ifndef _REGISTRY_MANAGER_H_
#define _REGISTRY_MANAGER_H_

#include "util/util_mysql.h"
#include "util/util_thread.h"
using namespace mfw;

enum RegServerStatus
{
	RegServerStatus_Active = 0,
	RegServerStatus_Inactive = 1,
};

struct RegServiceConf
{
	string		sService;
	string		sEndpoint;
};

struct RegServerConf
{
	string		sAppName;
	string		sServerName;
	string		sDivision;
	string		sNode;
	RegServerStatus	iStatus;
	vector<RegServiceConf> vService;

	bool isActive() const { return iStatus == RegServerStatus_Active; }
	const RegServiceConf *findService(const string &sService) const;
};

class RegistryManager
{
public:
	RegistryManager(): m_bDataLoaded(false) {}
	void initialize(const CDBConf &dbconf);
	int32_t reload();

	bool isReady() const { return m_bDataLoaded; }
	bool getServiceEndpoint(const string &sObj, const string &sDivision, vector<string> &vActiveEps, vector<string> &vInactiveEps);
	bool addServiceEndpoint(const string &sObj, const string &sDivision, const string &sEp);
	bool removeServiceEndpoint(const string &sObj, const string &sDivision, const string &sEp);

private:
	static string getLookupKey(const string &sAppName, const string &sServerName, const string &sDivision);
	static void extractServiceEndpoint(const vector<RegServerConf> &vRegServerConf, const string &sService, vector<string> &vActiveEps, vector<string> &vInactiveEps);

private:
	CMysql m_mysql;
	CMutex m_mutex;
	bool m_bDataLoaded;
	map<string, map<string, vector<string> > > m_mAppDivisionServer; // app => division => server
	map<string, map<string, vector<string> > > m_mAppServerDivision; // app => server => division
	map<string, vector<RegServerConf> > m_mServerNodes; //app!server!division => servers
};

extern RegistryManager g_stRegistryManager;

#endif
