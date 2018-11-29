#ifndef _MFW_ADMIN_SERVICE_H_
#define _MFW_ADMIN_SERVICE_H_

#include "service/Admin.h"

namespace mfw
{

class AdminService : public Admin
{
public:
    virtual void initialize();
    virtual void destroy();

	virtual void shutdown();
	virtual string notify(const string &command);
};
}

#endif


