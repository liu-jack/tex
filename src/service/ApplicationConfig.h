#ifndef _MFW_APPLICATION_CONFIG_H_
#define _MFW_APPLICATION_CONFIG_H_

#include <stdint.h>
#include <string>
#include <vector>
using namespace std;

namespace mfw
{

enum
{
	SYNC_CALL_DEFAULT_TIMEOUT = 3000,
	SYNC_CALL_MIN_TIMEOUT = 100,
	ASYNC_CALL_DEFAULT_TIMEOUT = 5000,
	ASYNC_CALL_MIN_TIMEOUT = 100,
	CONNECT_DEFAULT_TIMEOUT = 1000,
	CONNECT_MIN_TIMEOUT = 100,
	CONNECT_MAX_TIMEOUT = 5000,
	DEFAULT_ENDPOINT_REFRESH_INTERVAL = 60 * 1000,

	MAX_PENDING_ADAPTER_QUEUE = 1000,

	MAX_OBJECT_PROXY_NUM = 10000,

	TCP_RECV_BUFFER_SIZE = 64 * 1024,
	TCP_RECV_MAX_RESERVE_SIZE = 64 * 1024,
	TCP_RECV_MIN_RESERVE_SIZE = 1024,
	TCP_SEND_MAX_RESERVE_SIZE = 64 * 1024,
	TCP_SEND_MIN_RESERVE_SIZE = 1024,
	UDP_RECV_BUFFER_SIZE = 64 * 1024,

	MAX_PACKET_SIZE = 100 * 1024 * 1024,

	INVOKE_STABILITY_CHECK_INTERVAL = 60,
	INVOKE_STABILITY_MIN_FAIL_INVOKE_NUM = 2,
	INVOKE_STABILITY_FAIL_RATION100 = 50,
	INVOKE_CONTINUOUS_FAIL_CHECK_INTERVAL = 5,
	INVOKE_CONTINUOUS_FAIL_INVOKE_NUM = 5,
	INVOKE_RETRY_INTERVAL = 30,
	INVOKE_TIMEOUT_CHECK_INTERVAL = 100,

	DEFAULT_MAX_SERVER_CONNECTION = 1024,
	DEFAULT_MAX_SERVER_QUEUE_CAPACITY = 10 * 1024,
	DEFAULT_MAX_SERVER_QUEUE_TIMEOUT = 5 * 1000,
	MIN_SERVER_CONNECTION_IDLE_TIME = 2 * 1000,
};

struct ConnectorConfig
{
	bool		bIsSetEnabled;
	string		sDivision;

	string		sLocator;
	uint32_t	iEndpointRefreshInterval;
	uint32_t	iSyncInvokeTimeout;
	uint32_t	iAsyncInvokeTimeout;
	uint32_t	iConnectTimeout;
	uint32_t	iAsyncThreadNum;

	string		sModuleName;

	ConnectorConfig() : bIsSetEnabled(false),
			iEndpointRefreshInterval(DEFAULT_ENDPOINT_REFRESH_INTERVAL),
			iSyncInvokeTimeout(SYNC_CALL_DEFAULT_TIMEOUT),
			iAsyncInvokeTimeout(ASYNC_CALL_DEFAULT_TIMEOUT),
			iConnectTimeout(CONNECT_DEFAULT_TIMEOUT),
			iAsyncThreadNum(1)
	{}
};

struct ServiceConfig
{
	string		sService;
	string		sEndpoint;
	string		sProtocol;
	uint32_t	iThreadNum;
	uint32_t	iMaxConnection;
	uint32_t	iMaxQueue;
	uint32_t	iQueueTimeout;

	ServiceConfig() : iThreadNum(1), iMaxConnection(DEFAULT_MAX_SERVER_CONNECTION), iMaxQueue(DEFAULT_MAX_SERVER_QUEUE_CAPACITY), iQueueTimeout(DEFAULT_MAX_SERVER_QUEUE_TIMEOUT) {}
};

struct ApplicationConfig
{
	string		sAppName;
	string		sServerName;
	uint32_t	iLoopInterval;

	string		sLogPath;
	uint64_t	iLogSize;
	uint32_t	iLogNum;
	string		sLogLevel;
	string		sFrameworkLogLevel;
	string		sRemoteLogObj;
	string		sRemoteGlobalLogObj;

	string		sLocalAdminObj;

	ConnectorConfig stCommConfig;
	vector<ServiceConfig> vServiceConfig;

	ApplicationConfig() : iLoopInterval(0), iLogSize(100 * 1024 * 1024), iLogNum(10) {}
};

struct ServerConfig
{
    static string Application;
    static string ServerName;
};

struct ClientConfig
{
	static string ModuleName;
	static bool SetOpen;
	static string SetDivision;
};

extern ApplicationConfig g_stApplicationConfig;

void loadApplicationConfig(const string &sFile);

}

#endif
