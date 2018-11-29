#ifndef __MFW_SDP_CURRENT_H_
#define __MFW_SDP_CURRENT_H_

#include "service/MfwPacket.h"
#include "service/ReturnCode.h"
#include "service/NetServer.h"

namespace mfw
{

class ServiceHandle;

class SdpCurrent
{
public:
    SdpCurrent(ServiceHandle *pServiceHandle);

    void initialize(const tagRecvData &stRecvData);
    void initializeClose(const tagRecvData &stRecvData);

    BindAdapter *getBindAdapter() { return m_pBindAdapter; }
    const string &getIp() const { return m_sIp; }
    uint16_t getPort() const { return m_iPort; }
	uint32_t getUid() const { return m_iUid; }
    bool isResponse() const { return m_bResponse; }
    void setResponse(bool value) { m_bResponse = value; }
    void setResponseContext(const map<string, string> &context) { m_responseContext = context;}
    const map<string, string> &getResponseContext() const { return m_responseContext; }
    map<string, string> &getResponseContext() { return m_responseContext; }

    const string &getServiceName() const { return m_request.sServiceName; }
    const string &getFuncName() const { return m_request.sFuncName; }
    const string &getRequestBuffer() const { return m_request.sReqPayload; }
    uint32_t getRequestTimeout() const { return m_request.iTimeout; }
    const map<string, string> &getContext() const { return m_request.context; }
    uint32_t getRequestId() const { return m_request.iRequestId; }
    bool isOneWay() const { return m_request.bIsOneWay; }

    void close();
    void sendMfwResponse(int32_t iMfwRet, const string &sRspPayload = "");
    void sendNoMfwResponse(const string &sData);

    void setUserData1(uint64_t val) { m_iUserData1 = val; }
    uint64_t getUserData1() const { return m_iUserData1; }
    void setUserData2(uint64_t val) { m_iUserData2 = val; }
    uint64_t getUserData2() const { return m_iUserData2; }

protected:
    ServiceHandle *m_pServiceHandle;
    BindAdapter *m_pBindAdapter;
    uint64_t m_iBegintime;

    string m_sIp;
    uint16_t m_iPort;
    uint32_t m_iUid;
    RequestPacket m_request;
    bool m_bResponse;
    map<string, string> m_responseContext;

    uint64_t m_iUserData1;
    uint64_t m_iUserData2;
};

}
#endif
