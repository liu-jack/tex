#ifndef _WEBSOCKET_CONFIG_H_
#define _WEBSOCKET_CONFIG_H_

#include <stdint.h>
#include <string>
using namespace std;

struct WebsocketConfig
{

};

extern WebsocketConfig g_stWebsocketConfig;

int32_t loadWebsocketConfig();

#endif
