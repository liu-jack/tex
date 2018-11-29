#ifndef _REGISTRY_SERVER_H
#define _REGISTRY_SERVER_H

#include "service/Application.h"
using namespace mfw;

class RegistryServer : public Application
{
protected:
    virtual bool initialize();
    virtual void destroyApp();
};

#endif
