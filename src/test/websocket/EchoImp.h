#ifndef _ECHO_IMP_H_
#define _ECHO_IMP_H_

#include "Echo.h"

class EchoImp : public Test::Echo
{
public:
	virtual void initialize();
    virtual void destroy();
};
#endif
