#ifndef _LOG_SERVER_H
#define _LOG_SERVER_H

#include "service/Application.h"
using namespace mfw;

class LogServer : public Application
{
protected:
    virtual bool initialize();
    virtual void destroyApp();
};

#endif

