#ifndef _MFW_MESSAGE_H_
#define _MFW_MESSAGE_H_

#include "service/MfwPacket.h"
#include "service/Global.h"
#include "util/util_thread.h"

namespace mfw
{

struct ReqMessage
{
    enum CallType
    {
        SYNC_CALL,
        ASYNC_CALL,
        ONE_WAY,
    };

    enum MsgStatus
    {
        REQ_REQ,
        REQ_RSP,
        REQ_TIME,
        REQ_EXC
    };

    MsgStatus                   eMsgStatus;
    CallType                    eCallType;
    bool                        bFromRpc;
    ServiceProxyCallbackPtr     callback;
    RequestPacket               request;
    ResponsePacket              response;
    string                      sReqData;
    uint64_t                    iBeginTime;
    bool                        bHash;
    uint64_t                    iHashCode;
    CNotifier 					*pMonitor;
    bool                        bMonitorFin;
    ObjectProxy					*pObjectProxy;
    AdapterProxy				*pAdapterProxy;

    ReqMessage() :
    	eMsgStatus(ReqMessage::REQ_REQ),
    	eCallType(SYNC_CALL),
    	bFromRpc(false),
    	iBeginTime(0),
    	bHash(false),
    	iHashCode(0),
    	pMonitor(NULL),
    	bMonitorFin(false),
    	pObjectProxy(NULL),
    	pAdapterProxy(NULL)
    {
    }

    ~ReqMessage()
    {
        if (pMonitor != NULL)
        {
            delete pMonitor;
        }
    }

    bool isSyncCall() const { return eCallType == SYNC_CALL; }
    bool isAsyncCall() const { return eCallType == ASYNC_CALL; }
    bool isOnewayCall() const { return eCallType == ONE_WAY; }
};

}

#endif
