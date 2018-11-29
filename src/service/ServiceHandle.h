#ifndef _MFW_SERVICE_HANDLE_H_
#define _MFW_SERVICE_HANDLE_H_

#include "service/Service.h"

namespace mfw
{

class ServiceHandle : public Handle
{
public:
	~ServiceHandle();

protected:
	virtual bool initialize();
    virtual void handle(const tagRecvData &stRecvData);
    virtual void handleTimeout(const tagRecvData &stRecvData);
    virtual void handleOverload(const tagRecvData &stRecvData);
    virtual void handleClose(const tagRecvData &stRecvData);
    SdpCurrentPtr createCurrent(const tagRecvData &stRecvData);
    SdpCurrentPtr createCloseCurrent(const tagRecvData &stRecvData);
    void handleMfwProtocol(const SdpCurrentPtr &current);
    void handleNoMfwProtocol(const SdpCurrentPtr &current);

protected:
    ServicePtr m_service;
};

}

#endif
