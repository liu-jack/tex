#ifndef _WEBSOCKET_H_
#define _WEBSOCKET_H_

#include "service/Application.h"
using namespace mfw;

class Websocket : public Application
{
public:
    virtual bool initialize();
    virtual void destroyApp();
};

#endif
