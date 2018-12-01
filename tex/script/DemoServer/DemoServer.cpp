#include "DemoServer.h"
#include "DemoServiceImp.h"
#include "DemoServerConfig.h"
using namespace std;

DemoServer g_app;

void DemoServer::initialize()
{
    int32_t ret = loadDemoServerConfig();
    if (ret != 0) {
        LOG_ERROR("loadDemoServerConfig:" << ret);
        exit(-1);
    }

    addService<DemoServiceImp>(ServerConfig::Application + "." + ServerConfig::ServerName + ".DemoServiceObj");
}

void DemoServer::destroyApp()
{
}

int main(int argc, char *argv[])
{
    return g_app.main(argc, argv);
}
