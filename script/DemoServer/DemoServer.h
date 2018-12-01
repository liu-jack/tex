#ifndef _DEMOSERVER_H_
#define _DEMOSERVER_H_

#include "service/Application.h"
using namespace mfw;

class DemoServer : public Application
{
public:
    virtual bool initialize();
    virtual void destroyApp();
};

#endif
