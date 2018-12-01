#ifndef _MFW_APPLICATION_H_
#define _MFW_APPLICATION_H_

#include "service/NetServer.h"
#include "service/ServiceCreator.h"
#include "service/Connector.h"
#include <iostream>
#include <stdlib.h>
namespace mfw
{

class Application
{
public:
    virtual ~Application();

    int main(int argc, char *argv[]);
    NetServerPtr &getNetServer()
    {
        return _netServer;
    }
    ConnectorPtr &getConnector()
    {
        return _connector;
    }
    void terminate();

protected:
    virtual bool initialize() = 0;
    virtual void destroyApp() = 0;
    virtual void loop();

    template<typename T>
    void addService(const string &sServiceName)
    {
        ServiceCreatorManager::getInstance()->addService<T>(sServiceName);
    }

    template<typename T>
    void addService(const string &sServiceName, const protocol_functor &fn)
    {
        addService<T>(sServiceName);
        addServiceProtocol(sServiceName, fn);
    }

    void addServiceProtocol(const string &sServiceName, const protocol_functor &fn);

private:
    void parseConfig(int argc, char *argv[]);
    void initializeClient();
    void initializeServer();

private:
    NetServerPtr _netServer;
    ConnectorPtr _connector;
};

extern Application* g_app;

}

#endif

