#include "WebsocketConfig.h"
#include "util/util_config.h"
#include "util/util_log.h"
#include "service/Application.h"
using namespace mfw;

WebsocketConfig g_stWebsocketConfig;

int32_t loadWebsocketConfig()
{
    __TRY__
    CConfig config;
    config.parseFile(ServerConfig::ServerName + ".conf");

    return 0;
    __CATCH__
    return -1;
}
