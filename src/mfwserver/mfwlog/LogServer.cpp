#include "LogServer.h"
#include "LogImp.h"
#include "AsyncLogger.h"
#include "util/util_config.h"
using namespace mfw;

mfw::Application *mfw::g_app = new LogServer();

LogServerConfig g_stLogServerConfig;
CThread g_asyncLoggerThread("async_logger");

bool LogServer::initialize()
{
	CConfig config;
	config.parseFile(ServerConfig::ServerName + ".conf");
	g_stLogServerConfig.sLogPath = config.getCfg("/Main/LogPath");
	if (g_stLogServerConfig.sLogPath.empty())
	{
		g_stLogServerConfig.sLogPath = ".";
	}

	addService<LogImp>(ServerConfig::Application + "." + ServerConfig::ServerName + ".LogObj");
	g_asyncLoggerThread.setRoutine(asyncLoggerThread);
	g_asyncLoggerThread.start();

    return true;
}

void LogServer::destroyApp()
{
	g_asyncLoggerThread.terminate();
	g_asyncLoggerThread.join();
}

int main(int argc, char *argv[])
{
    return g_app->main(argc, argv);
}
