#include "HandleEcho.h"
#include "util/util_log.h"
#include "util/util_time.h"
#include "Test.h"

void HandleEcho::initialize()
{
    MFW_DEBUG("initialize");
}

void HandleEcho::destroy()
{
    MFW_DEBUG("destory");
}

int32_t HandleEcho::handleClose()
{
    MFW_DEBUG("websocket close");
}

int32_t HandleEcho::handleNoMfwRequest(const string &sReqPayload, string &sRspPayload)
{

    Test::Class c;
    uint64_t t1 = UtilTime::getNowUS();
    stringToSdp(sReqPayload, c);
    uint64_t t2 = UtilTime::getNowUS();
    MFW_DEBUG("unpack cost us:" << (t2-t1));

    Test::Teachers ts;
    stringToSdp(string(c.vData.begin(),c.vData.end()), ts);

    MFW_DEBUG("recv class:" << printSdp(c) << ", teachers:" << printSdp(ts));

    t1 = UtilTime::getNowUS();
    sRspPayload = sdpToString(c);
    t2 = UtilTime::getNowUS();
    MFW_DEBUG("pack cost us:" << (t2-t1));

    return 0;
}
