#include "RedisWrapper.h"
#include "util/util_string.h"
#include <iostream>
using namespace mfw::redis;

namespace mfw
{

void RedisProxy::initRedisProtocol()
{
	mfw::ClientSideProtocol protocol;
	protocol.requestFunc = NULL;
	protocol.responseFunc = redis_responseFunc;
	mfw_set_protocol(protocol);
}

}

namespace
{

int32_t decodeResponse_callBatch(const string &sRspPayload, vector<string> &vResult, RedisPrxCallback *cb = NULL)
{
	CRedisReplyDecoder decoder(sRspPayload);
	while (decoder.nextReply())
	{
		vResult.push_back(string(decoder.getContentBegin(), decoder.getContentEnd()));
	}

	if (cb != NULL)
	{
		cb->callback_callBatch(0, vResult);
		return 0;
	}
	return 0;
}

void encodeRequest_getString(CRedisProtocolPacker &packer, const string &sKey)
{
	packer.setCmdNum(1);
	packer.appendCmd("get", 1);
	packer.appendArg(sKey);
}

int32_t decodeResponse_getString(const string &sRspPayload, string &sValue, RedisPrxCallback *cb = NULL)
{
	CRedisReplyDecoder decoder(sRspPayload);
	if (!decoder.nextReply(RedisReplyType_String | RedisReplyType_Null))
	{
		if (cb != NULL)
		{
			cb->callback_getString_exception(-1);
			return -1;
		}
		throw std::runtime_error("getString: redis decode error");
	}

	int32_t ret = 0;
	RedisReply &reply = decoder.getCurReply();
	if (reply.isNull())
	{
		ret = RedisResult_NoData;
	}
	else
	{
		sValue.assign(reply.str, reply.len);
	}
	if (cb != NULL)
	{
		cb->callback_getString(ret, sValue);
		return 0;
	}
	return ret;
}

void encodeRequest_setString(CRedisProtocolPacker &packer, const string &sKey, const string &sValue)
{
	packer.setCmdNum(1);
	packer.appendCmd("set", 2);
	packer.appendArg(sKey);
	packer.appendArg(sValue);
}

int32_t decodeResponse_setString(const string &sRspPayload, RedisPrxCallback *cb = NULL)
{
	CRedisReplyDecoder decoder(sRspPayload);
	if (!decoder.nextReply(RedisReplyType_Status))
	{
		if (cb != NULL)
		{
			cb->callback_setString_exception(-1);
			return -1;
		}
		throw std::runtime_error("setString: redis decode error");
	}

	if (cb != NULL)
	{
		cb->callback_setString(0);
		return 0;
	}
	return 0;
}

void encodeRequest_delString(CRedisProtocolPacker &packer, const string &sKey)
{
	packer.setCmdNum(1);
	packer.appendCmd("del", 1);
	packer.appendArg(sKey);
}

int32_t decodeResponse_delString(const string &sRspPayload, RedisPrxCallback *cb = NULL)
{
	CRedisReplyDecoder decoder(sRspPayload);
	if (!decoder.nextReply(RedisReplyType_Integer))
	{
		if (cb != NULL)
		{
			cb->callback_delString_exception(-1);
			return -1;
		}
		throw std::runtime_error("delString: redis decode error");
	}

	if (cb != NULL)
	{
		cb->callback_delString(0);
		return 0;
	}
	return 0;
}

void encodeRequest_getStringBatch(CRedisProtocolPacker &packer, const vector<string> &vKey)
{
	string sAllKeySdp = sdpToString(vKey);
	packer.setCmdNum(2);
	packer.appendCmd("echo", 1);
	packer.appendArg(sAllKeySdp);

	packer.appendCmd("mget", vKey.size());
	for (unsigned i = 0; i < vKey.size(); ++i)
	{
		const string &sKey = vKey[i];
		packer.appendArg(sKey);
	}
}

int32_t decodeResponse_getStringBatch(const string &sRspPayload, map<string, string> &mKeyValue, RedisPrxCallback *cb = NULL)
{
	CRedisReplyDecoder decoder(sRspPayload);
	if (!decoder.nextReply(RedisReplyType_String))
	{
		if (cb != NULL)
		{
			cb->callback_getStringBatch_exception(-1);
			return -1;
		}
		throw std::runtime_error("getStringBatch: redis decode error 1");
	}

	string sAllKeySdp = string(decoder.getCurReply().str, decoder.getCurReply().len);
	vector<string> vKey;
	stringToSdp(sAllKeySdp, vKey);

	if (!decoder.nextReply(RedisReplyType_Array) || decoder.getCurReply().elements != (int32_t)vKey.size())
	{
		if (cb != NULL)
		{
			cb->callback_getStringBatch_exception(-1);
			return -1;
		}
		throw std::runtime_error("getStringBatch: redis decode error 2");
	}

	RedisReply &reply = decoder.getCurReply();
	for (unsigned i = 0; i < vKey.size(); ++i)
	{
		const string &sKey = vKey[i];
		RedisReply &r = reply.element[i];
		if (r.isString())
		{
			mKeyValue[sKey].assign(r.str, r.len);
		}
	}

	if (cb != NULL)
	{
		cb->callback_getStringBatch(0, mKeyValue);
		return 0;
	}
	return 0;
}

void encodeRequest_setStringBatch(CRedisProtocolPacker &packer, const map<string, string> &mSetValue)
{
	packer.setCmdNum(1);
	packer.appendCmd("mset", mSetValue.size() * 2);
	for (map<string, string>::const_iterator first = mSetValue.begin(), last = mSetValue.end(); first != last; ++first)
	{
		packer.appendArg(first->first);
		packer.appendArg(first->second);
	}
}

int32_t decodeResponse_setStringBatch(const string &sRspPayload, RedisPrxCallback *cb = NULL)
{
	CRedisReplyDecoder decoder(sRspPayload);
	if (!decoder.nextReply(RedisReplyType_Status))
	{
		if (cb != NULL)
		{
			cb->callback_setStringBatch_exception(-1);
			return -1;
		}
		throw std::runtime_error("setStringBatch: redis decode error");
	}

	if (cb != NULL)
	{
		cb->callback_setStringBatch(0);
		return 0;
	}
	return 0;
}

void encodeRequest_delStringBatch(CRedisProtocolPacker &packer, const vector<string> &vDelKey)
{
	packer.setCmdNum(1);
	packer.appendCmd("del", vDelKey.size());
	for (unsigned i = 0; i < vDelKey.size(); ++i)
	{
		const string &sKey = vDelKey[i];
		packer.appendArg(sKey);
	}
}

int32_t decodeResponse_delStringBatch(const string &sRspPayload, RedisPrxCallback *cb = NULL)
{
	CRedisReplyDecoder decoder(sRspPayload);
	if (!decoder.nextReply(RedisReplyType_Integer))
	{
		if (cb != NULL)
		{
			cb->callback_delStringBatch_exception(-1);
			return -1;
		}
		throw std::runtime_error("delStringBatch: redis decode error");
	}

	if (cb != NULL)
	{
		cb->callback_delStringBatch(0);
		return 0;
	}
	return 0;
}

void encodeRequest_incr(CRedisProtocolPacker &packer, const string &sKey, int64_t iNum)
{
	packer.setCmdNum(1);
	packer.appendCmd("incrby", 2);
	packer.appendArg(sKey);
	packer.appendArg(UtilString::tostr(iNum));
}

int32_t decodeResponse_incr(const string &sRspPayload, int64_t &iValue, RedisPrxCallback *cb = NULL)
{
	CRedisReplyDecoder decoder(sRspPayload);
	if (!decoder.nextReply(RedisReplyType_Integer))
	{
		if (cb != NULL)
		{
			cb->callback_incr_exception(-1);
			return -1;
		}
		throw std::runtime_error("incr: redis decode error");
	}

	RedisReply &reply = decoder.getCurReply();
	iValue = reply.integer;

	if (cb != NULL)
	{
		cb->callback_incr(0, iValue);
		return 0;
	}
	return 0;
}

void encodeRequest_decr(CRedisProtocolPacker &packer, const string &sKey, int64_t iNum)
{
	packer.setCmdNum(1);
	packer.appendCmd("decrby", 2);
	packer.appendArg(sKey);
	packer.appendArg(UtilString::tostr(iNum));
}

int32_t decodeResponse_decr(const string &sRspPayload, int64_t &iValue, RedisPrxCallback *cb = NULL)
{
	CRedisReplyDecoder decoder(sRspPayload);
	if (!decoder.nextReply(RedisReplyType_Integer))
	{
		if (cb != NULL)
		{
			cb->callback_decr_exception(-1);
			return -1;
		}
		throw std::runtime_error("decr: redis decode error");
	}

	RedisReply &reply = decoder.getCurReply();
	iValue = reply.integer;

	if (cb != NULL)
	{
		cb->callback_decr(0, iValue);
		return 0;
	}
	return 0;
}

}
