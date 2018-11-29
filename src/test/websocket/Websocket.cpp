#include "Websocket.h"
#include "HandleEcho.h"
#include "WebsocketConfig.h"
#include "EchoImp.h"

using namespace std;

mfw::Application *mfw::g_app = new Websocket();

bool Websocket::initialize()
{
	int32_t ret = loadWebsocketConfig();
	if (ret != 0)
	{
		LOG_ERROR("loadWebsocketConfig:" << ret);
        return false;
	}

	addService<EchoImp>(ServerConfig::Application + "." + ServerConfig::ServerName + ".EchoObj");
	addService<HandleEcho>(ServerConfig::Application + "." + ServerConfig::ServerName + ".HandleEcho");

    return true;
}

void Websocket::destroyApp()
{
}

int main(int argc, char *argv[])
{
	return g_app->main(argc, argv);
}
