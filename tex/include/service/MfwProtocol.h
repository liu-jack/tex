#ifndef _MFW_PROTOCOL_H_
#define _MFW_PROTOCOL_H_

#include <string>
#include <list>
#include "service/MfwPacket.h"
using namespace std;

namespace mfw
{

class ServerSideProtocol
{
public:
    static int mfw_protocol(const char *pInBegin, const char *pInEnd, const char *&pNextIn, string &sOut);
};

class ClientSideProtocol
{
public:
    ClientSideProtocol() : requestFunc(mfwRequest), responseFunc(mfwResponse) {}

    static void mfwRequest(const RequestPacket &request, string &buff);
    static size_t mfwResponse(const char *recvBuffer, size_t length, list<ResponsePacket> &done);

public:
    void (*requestFunc)(const RequestPacket &request, string &buff);
    size_t (*responseFunc)(const char *recvBuffer, size_t length, list<ResponsePacket> &done);
};

}
#endif
