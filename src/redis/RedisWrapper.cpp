#include "RedisWrapper.h"
#include "util/util_atomic.h"
#include <list>
#include <iostream>

namespace mfw
{
namespace redis
{

bool RedisReply::initType(char c)
{
    if (c == '+') {
        type = RedisReplyType_Status;
    } else if (c == '-') {
        type = RedisReplyType_Error;
    } else if (c == '$') {
        type = RedisReplyType_String;
    } else if (c == '*') {
        type = RedisReplyType_Array;
    } else if (c == ':') {
        type = RedisReplyType_Integer;
    } else {
        return false;
    }
    return true;
}

void RedisReply::print()
{
    cout << "type: " << type << ", val: ";
    if (isNull()) {
        cout << "(nil)";
    } else if (isStatus() || isError() || isString()) {
        cout << string(str, str + len);
    } else if (isInteger()) {
        cout << integer;
    } else if (isArray()) {
        cout << "elements " << elements << endl;
        for (int32_t i = 0; i < elements; ++i)
            element[i].print();
    }
    cout << endl;
}

void CRedisReplyDecoder::init(const char *pBegin, const char *pEnd)
{
    m_pBegin = pBegin;
    m_pEnd = pEnd;
    m_pContentBegin = NULL;
    m_pContentEnd = NULL;
    m_pNextBegin = m_pBegin;
    m_res = DecodeRes_OK;
    m_curReply.reset();
}

bool CRedisReplyDecoder::nextReply()
{
    m_curReply.reset();
    const char *pNextBegin = NULL;
    m_res = decode(m_pNextBegin, m_pEnd, m_curReply, pNextBegin);
    if (m_res != DecodeRes_OK) {
        return false;
    }
    m_pContentBegin = m_pNextBegin;
    m_pContentEnd = pNextBegin;
    m_pNextBegin = pNextBegin;
    return true;
}

bool CRedisReplyDecoder::nextReply(uint32_t iTypeMask)
{
    if (nextReply() && (getCurReply().type & iTypeMask) != 0) {
        return true;
    }
    return false;
}

CRedisReplyDecoder::DecodeRes CRedisReplyDecoder::decode(const char *pBegin, const char *pEnd, RedisReply &reply, const char *&pNextBegin)
{
    if (pBegin >= pEnd) {
        return DecodeRes_Less;
    }

    if (!reply.initType(*pBegin)) {
        return DecodeRes_Err;
    }

    if (reply.isStatus() || reply.isError()) {
        const char *pCrLf = NULL;
        DecodeRes res = seek_crlf(pBegin + 1, pEnd, pCrLf);
        if (res != DecodeRes_OK) {
            return res;
        }

        reply.str = pBegin + 1;
        reply.len = pCrLf - reply.str;
        pNextBegin = pCrLf + 2;
    } else if (reply.isString() || reply.isArray() || reply.isInteger()) {
        const char *pCrLf = NULL;
        DecodeRes res = seek_crlf(pBegin + 1, pEnd, pCrLf);
        if (res != DecodeRes_OK) {
            return res;
        }

        long long value = strtoll(pBegin + 1, NULL, 10);
        if (reply.isString()) {
            reply.len = value;
            if (reply.len < 0) {
                reply.type = RedisReplyType_Null;
                reply.str = NULL;
                pNextBegin = pCrLf + 2;
            } else if (reply.len == 0) {
                reply.str = NULL;
                pNextBegin = pCrLf + 2;
            } else {
                reply.str = pCrLf + 2;
                if (reply.str + reply.len + 2 > pEnd) {
                    return DecodeRes_Less;
                }
                if (*(reply.str + reply.len) != '\r' || *(reply.str + reply.len + 1) != '\n') {
                    return DecodeRes_Err;
                }
                pNextBegin = reply.str + reply.len + 2;
            }
        } else if (reply.isArray()) {
            reply.elements = value;
            if (reply.elements < 0) {
                reply.type = RedisReplyType_Null;
                pNextBegin = pCrLf + 2;
            } else if (reply.elements == 0) {
                pNextBegin = pCrLf + 2;
            } else {
                const char *next = pCrLf + 2;
                for (int32_t i = 0; i < reply.elements; ++i) {
                    RedisReply r;
                    DecodeRes res = decode(next, pEnd, r, next);
                    if (res != DecodeRes_OK) {
                        return res;
                    }
                    reply.element.push_back(r);
                }
                pNextBegin = next;
            }
        } else {
            reply.integer = value;
            pNextBegin = pCrLf + 2;
        }
    }
    return DecodeRes_OK;
}

CRedisReplyDecoder::DecodeRes CRedisReplyDecoder::seek_crlf(const char *pBegin, const char *pEnd, const char *&pResult)
{
    pResult = (const char *)memchr(pBegin, '\r', pEnd - pBegin);
    if (pResult == NULL || pResult + 1 == pEnd) {
        return DecodeRes_Less;
    }
    if (*(pResult + 1) != '\n') {
        return DecodeRes_Err;
    }
    return DecodeRes_OK;
}

static CAtomic<uint32_t> g_redisSeq;
static uint32_t generateRedisSeq()
{
    uint32_t iRequestId = 0;
    do {
        iRequestId = g_redisSeq.inc();
    } while (iRequestId == 0);
    return iRequestId;
}

void CRedisProtocolPacker::appendCmdArg(const vector<string> &vCmdArg)
{
    if (vCmdArg.size() == 0) {
        return;
    }
    appendCmd(vCmdArg[0], vCmdArg.size() - 1);
    for (unsigned i = 1; i < vCmdArg.size(); ++i) {
        appendArg(vCmdArg[i]);
    }
}

void CRedisProtocolPacker::appendCmdArg(const vector<vector<string> > &vAllCmdArg)
{
    for (unsigned i = 0; i < vAllCmdArg.size(); ++i) {
        appendCmdArg(vAllCmdArg[i]);
    }
}

void CRedisProtocolPacker::appendCmd(const string &sCmd, uint32_t iArgNum)
{
    if (m_iCurCmdNum >= m_iCmdNum) {
        throw std::runtime_error("redis request packer: too many cmd");
    }
    if (m_iCurLeftArgNum != 0) {
        throw std::runtime_error("redis request packer: missing arg");
    }

    char buf[64];
    if (m_iRequestId == 0) {
        m_iRequestId = generateRedisSeq();
        snprintf(buf, sizeof(buf), "echo mfw,%u,%u\r\n", m_iRequestId, m_iCmdNum);
        m_sCmdData.append(buf);
    }

    ++m_iCurCmdNum;
    m_iCurLeftArgNum = iArgNum + 1;

    snprintf(buf, sizeof(buf), "*%u\r\n", m_iCurLeftArgNum);
    m_sCmdData.append(buf);
    appendArg(sCmd);
}

void CRedisProtocolPacker::appendArg(const string &sArg)
{
    if (m_iCurLeftArgNum == 0) {
        throw std::runtime_error("redis request packer: too many arg");
    }

    char buf[64];
    snprintf(buf, sizeof(buf), "$%u\r\n", (uint32_t)sArg.size());
    m_sCmdData.append(buf);
    m_sCmdData.append(sArg);
    m_sCmdData.append("\r\n");
    --m_iCurLeftArgNum;
}

void CRedisProtocolPacker::appendArg(const vector<string> &vArgs)
{
    for (unsigned i = 0; i < vArgs.size(); ++i) {
        appendArg(vArgs[i]);
    }
}

const string &CRedisProtocolPacker::getData() const
{
    if (m_iCurCmdNum != m_iCmdNum || m_iCmdNum == 0) {
        throw std::runtime_error("redis request packer: missing cmd");
    }
    if (m_iCurLeftArgNum != 0) {
        throw std::runtime_error("redis request packer: missing arg");
    }
    return m_sCmdData;
}

size_t redis_responseFunc(const char *recvBuffer, size_t length, list<mfw::ResponsePacket> &done)
{
    const char *pBegin = recvBuffer;
    const char *pEnd = recvBuffer + length;

    while (pBegin < pEnd) {
        CRedisReplyDecoder decoder(pBegin, pEnd);
        if (!decoder.nextReply(RedisReplyType_String)) {
            if (decoder.isNeedMore()) {
                return pBegin - recvBuffer;
            }
            throw std::runtime_error("redis: mfw header");
        }

        uint32_t iRequestId = 0;
        uint32_t iCmdNum = 0;
        if (sscanf(decoder.getCurReply().str, "mfw,%u,%u", &iRequestId, &iCmdNum) != 2) {
            throw std::runtime_error("redis: mfw header format");
        }

        const char *pContentBegin = decoder.getNextBegin();
        for (uint32_t i = 0; i < iCmdNum; ++i) {
            if (!decoder.nextReply()) {
                if (decoder.isNeedMore()) {
                    return pBegin - recvBuffer;
                }
                throw std::runtime_error("redis: cmd reply");
            }
        }

        mfw::ResponsePacket rsp;
        rsp.iRequestId = iRequestId;
        done.push_back(rsp);
        done.back().sRspPayload.assign(pContentBegin, decoder.getNextBegin());

        pBegin = decoder.getNextBegin();
    }
    return pBegin - recvBuffer;
}

}
}
