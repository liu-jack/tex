#ifndef _SDP_REDIS_WRAPPER_H_
#define _SDP_REDIS_WRAPPER_H_

#include <string>
#include <vector>
#include <list>
#include "service/MfwPacket.h"
using namespace std;

namespace mfw
{
namespace redis
{

enum RedisReplyType
{
	RedisReplyType_Null 	= (1 << 0),
	RedisReplyType_Status	= (1 << 1),
	RedisReplyType_Error	= (1 << 2),
	RedisReplyType_String	= (1 << 3),
	RedisReplyType_Array	= (1 << 4),
	RedisReplyType_Integer	= (1 << 5),
};

struct RedisReply
{
	RedisReplyType type;
    int64_t integer;
    int32_t len;
    const char *str;
    int32_t elements;
    vector<RedisReply> element;

    RedisReply() : type(RedisReplyType_Null), integer(0), len(0), str(NULL), elements(0) {}
    bool initType(char c);
    bool isNull() const { return type == RedisReplyType_Null; }
    bool isStatus() const { return type == RedisReplyType_Status; }
    bool isError() const { return type == RedisReplyType_Error; }
    bool isString() const { return type == RedisReplyType_String; }
    bool isArray() const { return type == RedisReplyType_Array; }
    bool isInteger() const { return type == RedisReplyType_Integer; }
    void reset() { type = RedisReplyType_Null; integer = 0; len = 0; str = NULL; elements = 0; element.clear(); }
    void print();
};

class CRedisReplyDecoder
{
	enum DecodeRes
	{
		DecodeRes_OK,
		DecodeRes_Err,
		DecodeRes_Less,
	};

public:
	explicit CRedisReplyDecoder(const string &sData) { init(sData.c_str(), sData.c_str() + sData.size()); }
	CRedisReplyDecoder(const char *pBegin, const char *pEnd) { init(pBegin, pEnd); }
	void init(const char *pBegin, const char *pEnd);

	bool nextReply();
	bool nextReply(uint32_t iTypeMask);
	bool isNeedMore() { return m_res == DecodeRes_Less; }
	bool isEnd() const { return m_pNextBegin >= m_pEnd; }
	RedisReply &getCurReply() { return m_curReply; }
	const char *getContentBegin() const { return m_pContentBegin; }
	const char *getContentEnd() const { return m_pContentEnd; }
	const char *getNextBegin() const { return m_pNextBegin; }

private:
	static DecodeRes decode(const char *pBegin, const char *pEnd, RedisReply &reply, const char *&pNextBegin);
	static DecodeRes seek_crlf(const char *pBegin, const char *pEnd, const char *&pResult);

private:
	const char *m_pBegin;
	const char *m_pEnd;
	const char *m_pContentBegin;
	const char *m_pContentEnd;
	const char *m_pNextBegin;
	DecodeRes m_res;
	RedisReply m_curReply;
};

class CRedisProtocolPacker
{
public:
	CRedisProtocolPacker() : m_iRequestId(0), m_iCmdNum(0), m_iCurCmdNum(0), m_iCurLeftArgNum(0) {}
	explicit CRedisProtocolPacker(uint32_t iCmdNum) : m_iRequestId(0), m_iCmdNum(iCmdNum), m_iCurCmdNum(0), m_iCurLeftArgNum(0) {}

	void setCmdNum(uint32_t iCmdNum) { m_iCmdNum = iCmdNum; }
	void appendCmdArg(const vector<string> &vCmdArg);
	void appendCmdArg(const vector<vector<string> > &vAllCmdArg);

	void appendCmd(const string &sCmd, uint32_t iArgNum);
	void appendArg(const string &sArg);
	void appendArg(const vector<string> &vArgs);
	const string &getData() const;
	uint32_t getRequestId() const { return m_iRequestId; }

private:
	uint32_t m_iRequestId;
	uint32_t m_iCmdNum;
	uint32_t m_iCurCmdNum;
	uint32_t m_iCurLeftArgNum;
	string m_sCmdData;
};

size_t redis_responseFunc(const char *recvBuffer, size_t length, list<mfw::ResponsePacket> &done);

}
}

#endif
