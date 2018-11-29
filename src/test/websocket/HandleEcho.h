#ifndef _WEBSOCKET_ECHO_H_
#define _WEBSOCKET_ECHO_H_

#include "service/Service.h"

class HandleEcho : public mfw::Service
{
public:
    virtual void initialize();
    virtual void destroy();
    virtual int32_t handleClose();
    virtual int32_t handleNoMfwRequest(const string &sReqPayload, string &sRspPayload);
};

#endif
