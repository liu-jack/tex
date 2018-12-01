#include "service/MfwProtocol.h"
#include "service/ApplicationConfig.h"
#include "service/ReturnCode.h"
#include "service/NetServer.h"
#include "util/util_string.h"
#include "sdp/Sdp.h"
#include <iostream>

namespace mfw
{

int ServerSideProtocol::mfw_protocol(const char *pInBegin, const char *pInEnd, const char *&pNextIn, string &sOut)
{
    uint32_t len = pInEnd - pInBegin;
    if (len < sizeof(uint32_t)) {
        return NetServer::PACKET_LESS;
    }

    uint32_t iHeaderLen = ntohl(*(uint32_t*)pInBegin);
    if (iHeaderLen < sizeof(uint32_t) || iHeaderLen > MAX_PACKET_SIZE) {
        return NetServer::PACKET_ERR;
    }

    if (len < iHeaderLen) {
        return NetServer::PACKET_LESS;
    }

    sOut.assign(pInBegin + sizeof(uint32_t), pInBegin + iHeaderLen);
    pNextIn = pInBegin + iHeaderLen;
    return NetServer::PACKET_FULL;
}

void ClientSideProtocol::mfwRequest(const RequestPacket &request, string &buff)
{
    SdpPacker packer;
    packer.pack(request);

    uint32_t iHeaderLen = htonl(sizeof(uint32_t) + packer.getData().size());

    buff.clear();
    buff.reserve(sizeof(uint32_t) + packer.getData().size());
    buff.append((const char*)&iHeaderLen, sizeof(uint32_t));
    buff.append(packer.getData());
}

size_t ClientSideProtocol::mfwResponse(const char *recvBuffer, size_t length, list<ResponsePacket> &done)
{
    size_t pos = 0;
    while (pos < length) {
        uint32_t len = length - pos;
        if (len < sizeof(uint32_t)) {
            break;
        }

        uint32_t iHeaderLen = ntohl(*(uint32_t*)(recvBuffer + pos));
        if (iHeaderLen > MAX_PACKET_SIZE || iHeaderLen < sizeof(uint32_t)) {
            throw std::runtime_error("invalid packet length: " + UtilString::tostr(iHeaderLen));
        }

        if (len < iHeaderLen) {
            break;
        } else {
            SdpUnpacker unpacker(recvBuffer + pos + sizeof(uint32_t), iHeaderLen - sizeof(uint32_t));
            pos += iHeaderLen;

            ResponsePacket rsp;
            unpacker.unpack(rsp);

            done.push_back(rsp);
        }
    }

    return pos;
}

}

