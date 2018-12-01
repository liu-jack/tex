#include "service/ApplicationConfig.h"
#include "util/util_config.h"
#include "util/util_file.h"

namespace mfw
{

ApplicationConfig g_stApplicationConfig;

string ServerConfig::Application;
string ServerConfig::ServerName;

string ClientConfig::ModuleName = "unknown";
bool ClientConfig::SetOpen = false;
string ClientConfig::SetDivision = "";

void loadApplicationConfig(const string &sFile)
{
    CConfig config;
    config.parseFile(sFile);

    const CConfig &appcfg = config.getSubConfig("/mfw/application");
    const CConfig &servercfg = appcfg.getSubConfig("server");
    const CConfig &clientcfg = appcfg.getSubConfig("client");

    g_stApplicationConfig.sAppName = servercfg.getCfg("app");
    g_stApplicationConfig.sServerName = servercfg.getCfg("server");
    g_stApplicationConfig.iLoopInterval = servercfg.getCfg<uint32_t>("loop-interval", (uint32_t)-1);

    g_stApplicationConfig.sLogPath = servercfg.getCfg("logpath", "-");
    g_stApplicationConfig.iLogSize = UtilString::parseHumanReadableSize(servercfg.getCfg("logsize", "100M"));
    g_stApplicationConfig.iLogNum = servercfg.getCfg<uint32_t>("lognum", 10);
    g_stApplicationConfig.sLogLevel = servercfg.getCfg("loglevel", "DEBUG");
    g_stApplicationConfig.sFrameworkLogLevel = servercfg.getCfg("framework-loglevel", "INFO");
    g_stApplicationConfig.sRemoteLogObj = servercfg.getCfg("log", "");
    g_stApplicationConfig.sRemoteGlobalLogObj = servercfg.getCfg("global-log", g_stApplicationConfig.sRemoteLogObj);

    g_stApplicationConfig.sLocalAdminObj = servercfg.getCfg("admin", "");

    g_stApplicationConfig.stCommConfig.sDivision = appcfg.getCfg("setdivision", "");
    g_stApplicationConfig.stCommConfig.bIsSetEnabled = !g_stApplicationConfig.stCommConfig.sDivision.empty();

    g_stApplicationConfig.stCommConfig.sLocator = clientcfg.getCfg("locator", "");
    g_stApplicationConfig.stCommConfig.iEndpointRefreshInterval = clientcfg.getCfg<uint32_t>("refresh-endpoint-interval", DEFAULT_ENDPOINT_REFRESH_INTERVAL);
    g_stApplicationConfig.stCommConfig.iSyncInvokeTimeout = clientcfg.getCfg<uint32_t>("sync-invoke-timeout", SYNC_CALL_DEFAULT_TIMEOUT);
    g_stApplicationConfig.stCommConfig.iAsyncInvokeTimeout = clientcfg.getCfg<uint32_t>("async-invoke-timeout", ASYNC_CALL_DEFAULT_TIMEOUT);
    g_stApplicationConfig.stCommConfig.iConnectTimeout = clientcfg.getCfg<uint32_t>("connect-timeout", CONNECT_DEFAULT_TIMEOUT);
    g_stApplicationConfig.stCommConfig.iAsyncThreadNum = clientcfg.getCfg<uint32_t>("asyncthread", 3);
    g_stApplicationConfig.stCommConfig.sModuleName = g_stApplicationConfig.sAppName + "." + g_stApplicationConfig.sServerName;

    const map<string, CConfig> &mServiceCfg = servercfg.getAllSubConfig();
    for (map<string, CConfig>::const_iterator first = mServiceCfg.begin(), last = mServiceCfg.end(); first != last; ++first) {
        const CConfig &servicecfg = first->second;

        ServiceConfig stServiceConfig;
        stServiceConfig.sService = servicecfg.getCfg("service");
        stServiceConfig.sEndpoint = servicecfg.getCfg("endpoint");
        stServiceConfig.sProtocol = servicecfg.getCfg("protocol", "mfw");
        stServiceConfig.iThreadNum = servicecfg.getCfg<uint32_t>("threads", 1);
        stServiceConfig.iMaxConnection = servicecfg.getCfg<uint32_t>("maxconns", DEFAULT_MAX_SERVER_CONNECTION);
        stServiceConfig.iMaxQueue = servicecfg.getCfg<uint32_t>("queuecap", DEFAULT_MAX_SERVER_QUEUE_CAPACITY);
        stServiceConfig.iQueueTimeout = servicecfg.getCfg<uint32_t>("queuetimeout", DEFAULT_MAX_SERVER_QUEUE_TIMEOUT);
        g_stApplicationConfig.vServiceConfig.push_back(stServiceConfig);
    }
}

}
