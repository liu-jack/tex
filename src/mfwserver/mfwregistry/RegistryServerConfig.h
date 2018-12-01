#ifndef _REGISTRY_SERVER_CONFIG_H_
#define _REGISTRY_SERVER_CONFIG_H_

#include "util/util_mysql.h"
using namespace mfw;

struct RegistryServerConfig {
    CDBConf		stDBConf;
    uint32_t	iReloadInterval;
};

extern RegistryServerConfig g_stRegistryServerConfig;

#endif
