#ifndef _DEMOSERVER_CONFIG_H_
#define _DEMOSERVER_CONFIG_H_

#include <stdint.h>
#include <string>
using namespace std;

struct DemoServerConfig
{

};

extern DemoServerConfig g_stDemoServerConfig;

int32_t loadDemoServerConfig();

#endif
