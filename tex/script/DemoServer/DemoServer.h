#ifndef _DEMOSERVER_H_
#define _DEMOSERVER_H_

#include "service/Application.h"
using namespace mfw;

class DemoServer : public Application
{
public:
	virtual void initialize();
	virtual void destroyApp();
};

extern DemoServer g_app;

#endif
