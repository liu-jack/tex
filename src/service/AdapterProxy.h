#ifndef _MFW_ADAPTER_PROXY_H_
#define _MFW_ADAPTER_PROXY_H_

#include "service/ObjectProxy.h"
#include "service/Transport.h"
#include "service/Message.h"
#include "util/util_queue.h"
#include "util/util_network.h"

namespace mfw
{

class AdapterProxy
{
public:
    AdapterProxy(ObjectProxy *pObjectProxy, const CEndpoint &ep);
    ~AdapterProxy();

    ObjectProxy *getObjectProxy() { return m_pObjectProxy; }
    const CEndpoint &getEndpoint() const { return m_ep; }
    const string &getFullDesc() const { return m_sFullDesc; }

	void invoke(ReqMessage *msg);
    void doInvoke();
    void finishInvoke(ResponsePacket &rsp);
    void finishInvoke(ReqMessage *msg);
    void doTimeout(uint64_t iNowMS);

    bool checkActive(bool bForceConnect = false);

    void setConnTimeout(bool bConTimeout);
	bool isConnTimeout() { return m_bConnTimeout; }
    void setConnExc(bool bExc);
	bool isConnExc() { return m_bConnExc; }

private:
    void doTimeout(uint64_t iNowMS, CTimeQueue<ReqMessage *, uint64_t> &requestQueue);
    void setInactive();
    void updateStatus(bool bFail);

private:
    string m_sFullDesc;
    ObjectProxy *m_pObjectProxy;
    Transport *m_pTrans;
    CEndpoint m_ep;

    uint32_t m_iLastRequestId;
    map<uint32_t, ReqMessage *>	m_mRequestId2Msg;
    CTimeQueue<ReqMessage *, uint64_t> m_doingRequestQueue;
    CTimeQueue<ReqMessage *, uint64_t> m_pendingRequestQueue;

    uint32_t						m_iNextStabilityCheckTime;
    uint32_t						m_iTotalInvokeNum;
    uint32_t						m_iFailInvokeNum;

    uint32_t                        m_iContinuousFailCheckTime;
    uint32_t						m_iContinuousFailNum;

    bool m_bIsActive;
    uint32_t m_iNextRetryTime;

	bool m_bConnTimeout;
	bool m_bConnExc;
};

}
#endif
