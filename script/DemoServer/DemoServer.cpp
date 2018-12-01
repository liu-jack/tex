#include "DemoServer.h"
#include "DemoServiceImp.h"
#include "DemoServerConfig.h"
using namespace std;

mfw::Application* mfw::g_app = new DemoServer;

bool DemoServer::initialize()
{
    int32_t ret = loadDemoServerConfig();
    if (ret != 0) {
        LOG_ERROR("loadDemoServerConfig:" << ret);
        return false;
    }

    addService<DemoServiceImp>(ServerConfig::Application + "." + ServerConfig::ServerName + ".DemoServiceObj");
    return true;
}

void DemoServer::destroyApp()
{
}

int main(int argc, char *argv[])
{
    return g_app->main(argc, argv);
}
