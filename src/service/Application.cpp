#include "service/Application.h"
#include "service/MfwProtocol.h"
#include "service/AdminService.h"
#include "service/ServiceHandle.h"
#include "service/RemoteLog.h"
#include "util/util_option.h"
#include "util/util_file.h"
#include <signal.h>
#include <stdlib.h>
#include <iostream>
#include <unistd.h>

namespace mfw
{

static void sighandler(int /*sig*/)
{
    g_app->terminate();
}

Application::~Application()
{
    terminate();
}

void Application::parseConfig(int argc, char *argv[])
{
	COption option;
	option.parse(argc, argv);

	string sConfigFile = option.getOption("config");
	if (sConfigFile == "")
	{
		cerr << "Usage: " << argv[0] << " --mfw.conf" << endl;
		exit(0);
	}
	loadApplicationConfig(sConfigFile);

    ServerConfig::Application  		= g_stApplicationConfig.sAppName;
    ServerConfig::ServerName        = g_stApplicationConfig.sServerName;

	string sNoDaemon = option.getOption("nodaemon");
	if (sNoDaemon != "1")
	{
		int32_t iRet = daemon(1, 0);
		if (iRet != 0)
		{
			cerr << "start as daemon error: " << iRet << endl;
			exit(0);
		}
	}

    if (g_stApplicationConfig.sLogPath != "-")
	{
		string sLogPath = g_stApplicationConfig.sLogPath + "/" + ServerConfig::Application + "/";
		if (g_stApplicationConfig.stCommConfig.bIsSetEnabled)
		{
			sLogPath += g_stApplicationConfig.stCommConfig.sDivision + "/";
		}
		sLogPath += ServerConfig::ServerName;
		CLogManager::getInstance()->initLog(sLogPath, ServerConfig::Application + "." + ServerConfig::ServerName);
	}
	else
	{
		CLogManager::getInstance()->initLog(".", "-");
	}

    CLogManager::getInstance()->setLogLevel(g_stApplicationConfig.sLogLevel, g_stApplicationConfig.sFrameworkLogLevel);
    CLogManager::getInstance()->setRollLogInfo(CLogManager::getInstance()->getRollLog(), g_stApplicationConfig.iLogNum, g_stApplicationConfig.iLogSize);
}

void Application::initializeClient()
{
    _connector = ConnectorPtr(new Connector());
    _connector->setConfig(g_stApplicationConfig.stCommConfig);

    MfwRemoteLog::initRemoteLog(g_stApplicationConfig.sRemoteLogObj, g_stApplicationConfig.sRemoteGlobalLogObj);
}

void Application::initializeServer()
{
    _netServer = NetServerPtr(new NetServer());
	
    if (!g_stApplicationConfig.sLocalAdminObj.empty())
    {
    	string sServiceName = "AdminObj";
        BindAdapterPtr lsPtr(new BindAdapter(_netServer.get(), sServiceName));
        lsPtr->setEndpoint(g_stApplicationConfig.sLocalAdminObj);
        lsPtr->setHandle<ServiceHandle>(1);
        _netServer->bind(lsPtr);

        ServiceCreatorManager::getInstance()->addService<AdminService>(sServiceName);
    }

	for (unsigned i = 0; i < g_stApplicationConfig.vServiceConfig.size(); ++i)
	{
		const ServiceConfig &stServiceConfig = g_stApplicationConfig.vServiceConfig[i];

		BindAdapterPtr lsPtr(new BindAdapter(_netServer.get(), stServiceConfig.sService));
		lsPtr->setEndpoint(stServiceConfig.sEndpoint);
		lsPtr->setMaxConns(stServiceConfig.iMaxConnection);
		lsPtr->setQueueCapacity(stServiceConfig.iMaxQueue);
		lsPtr->setQueueTimeout(stServiceConfig.iQueueTimeout);
		lsPtr->setProtocolName(stServiceConfig.sProtocol);
		lsPtr->setHandle<ServiceHandle>(stServiceConfig.iThreadNum);
		_netServer->bind(lsPtr);
	}
}

void Application::addServiceProtocol(const string &sServiceName, const protocol_functor &fn)
{
	BindAdapterPtr pBindAdapter = _netServer->getNetThread()->getBindAdapter(sServiceName);
    if (!pBindAdapter)
    {
        throw runtime_error("service not exist: " + sServiceName);
    }
    pBindAdapter->setProtocol(fn);
}

void Application::loop()
{
}

void Application::terminate()
{
    if (_netServer)
    {
        _netServer->terminate();
    }
}

int Application::main(int argc, char *argv[])
{
    try
    {
        parseConfig(argc, argv);
        initializeClient();
        initializeServer();
        signal(SIGINT, sighandler);
        signal(SIGTERM, sighandler);
		signal(SIGPIPE, SIG_IGN);

        if (initialize()) {
            _netServer->launch();
		    while(_netServer->isTerminate() == false)
		    {
			    _netServer->waitNotify(g_stApplicationConfig.iLoopInterval);

			    loop();
		    }
        }

        destroyApp();
        _netServer.reset();

        // 结束远程日志,要不然会因为clientnet线程的退出而导致ServieProxy的同步调用无限等待
        CLogManager::getInstance()->setRemoteCallback(tr1::function<void(list<MfwLogData> &)>());	

        _connector.reset();
        CLogManager::getInstance()->finiLog();
    }
    catch (std::exception &e)
    {
    	MFW_ERROR("application exception: " << e.what());

        CLogManager::getInstance()->finiLog();

        cerr << e.what() << endl;
        return -1;
    }
    return 0;
}

}
