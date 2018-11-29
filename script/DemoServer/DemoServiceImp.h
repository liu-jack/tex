#ifndef _DEMOSERVICE_IMP_H_
#define _DEMOSERVICE_IMP_H_

#include "DemoService.h"

class DemoServiceImp : public DemoApp::DemoService
{
public:
	virtual void initialize();
    virtual void destroy();
};
#endif
