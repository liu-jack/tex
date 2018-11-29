#ifndef _MFW_SERVICE_H_
#define _MFW_SERVICE_H_

#include "service/ServiceProxy.h"
#include "service/SdpCurrent.h"

namespace mfw
{

class Service
{
public:
    virtual ~Service() {}

    void setServiceName(const string &sServiceName) { m_sServiceName = sServiceName; }
    const string &getServiceName() const { return m_sServiceName; }
    void setCurrent(const SdpCurrentPtr &current) { m_current = current; }
    void unsetCurrent() { m_current.reset(); }
    SdpCurrentPtr &getCurrent() { return m_current; }

    virtual void initialize() = 0;
    virtual void destroy() = 0;
    virtual int handleMfwRequest(string &/*sRspPayload*/) { return -1; }
    virtual int handleNoMfwRequest(const string &/*sReqPayload*/, string &/*sRspPayload*/) { return -1; }
	virtual int handleClose() { return -1; }

private:
    string m_sServiceName;
    SdpCurrentPtr m_current;
};

typedef tr1::shared_ptr<Service> ServicePtr;

class CallbackThreadData
{
public:
    static CallbackThreadData *getData();

    void setResponseContext(const map<string, string> &context) { m_responseContext = context; }
    map<string, string> &getResponseContext() { return m_responseContext; }
    void delResponseContext() { m_responseContext.clear(); }

private:
    map<string, string> m_responseContext;
};

}
#endif
