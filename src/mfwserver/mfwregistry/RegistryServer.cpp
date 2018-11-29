#include "RegistryServer.h"
#include "RegistryServerConfig.h"
#include "RegistryManager.h"
#include "QueryImp.h"
#include "util/util_config.h"

mfw::Application *mfw::g_app = new RegistryServer();

RegistryServerConfig g_stRegistryServerConfig;
CThread g_dbloaderThread("dbloader");

bool RegistryServer::initialize()
{
	CConfig config;
	config.parseFile(ServerConfig::ServerName + ".conf");
	g_stRegistryServerConfig.stDBConf.set(config.getSubConfig("/Main/DB").getAllKeyValue());
	g_stRegistryServerConfig.iReloadInterval = config.getCfg<uint32_t>("/Main/ReloadInterval");

	g_stRegistryManager.initialize(g_stRegistryServerConfig.stDBConf);
	g_dbloaderThread.setIntervalRoutine(tr1::bind(&RegistryManager::reload, &g_stRegistryManager), g_stRegistryServerConfig.iReloadInterval * 1000);
	g_dbloaderThread.start();

	addService<QueryImp>(ServerConfig::Application + "." + ServerConfig::ServerName + ".QueryObj");

    return true;
}

void RegistryServer::destroyApp()
{
}

int main(int argc, char *argv[])
{
	return g_app->main(argc, argv);
}
