#include "Redis.h"
#include "RedisInc.cpp"

namespace mfw
{
static map<string, int> __initRedisInterfaceLookup()
{
	map<string, int> m;
	m["call"] = 0;
	m["callBatch"] = 1;
	m["getString"] = 2;
	m["setString"] = 3;
	m["delString"] = 4;
	m["getStringBatch"] = 5;
	m["setStringBatch"] = 6;
	m["delStringBatch"] = 7;
	m["incr"] = 8;
	m["decr"] = 9;
	return m;
}
static const map<string, int> g_mRedisInterfaceLookup = __initRedisInterfaceLookup();

void RedisPrxCallback::onDispatch(mfw::ReqMessage *msg)
{
	map<string, int>::const_iterator it = g_mRedisInterfaceLookup.find(msg->request.sFuncName);
	if (it == g_mRedisInterfaceLookup.end()) throw std::runtime_error("proxy dispatch no such func");
	switch (it->second)
	{
	case 0:
		{
			if (msg->response.iMfwRet != 0)
			{
				callback_call_exception(msg->response.iMfwRet);
				return;
			}
			callback_call(0, msg->response.sRspPayload);
			return;
		}
	case 1:
		{
			if (msg->response.iMfwRet != 0)
			{
				callback_callBatch_exception(msg->response.iMfwRet);
				return;
			}

			vector<string> vResult;
			decodeResponse_callBatch(msg->response.sRspPayload, vResult, this);
			return;
		}
	case 2:
		{
			if (msg->response.iMfwRet != 0)
			{
				callback_getString_exception(msg->response.iMfwRet);
				return;
			}

			string sValue;
			decodeResponse_getString(msg->response.sRspPayload, sValue, this);
			return;
		}
	case 3:
		{
			if (msg->response.iMfwRet != 0)
			{
				callback_setString_exception(msg->response.iMfwRet);
				return;
			}
			decodeResponse_setString(msg->response.sRspPayload, this);
			return;
		}
	case 4:
		{
			if (msg->response.iMfwRet != 0)
			{
				callback_delString_exception(msg->response.iMfwRet);
				return;
			}
			decodeResponse_delString(msg->response.sRspPayload, this);
			return;
		}
	case 5:
		{
			if (msg->response.iMfwRet != 0)
			{
				callback_getStringBatch_exception(msg->response.iMfwRet);
				return;
			}

			map<string, string> mKeyValue;
			decodeResponse_getStringBatch(msg->response.sRspPayload, mKeyValue, this);
			return;
		}
	case 6:
		{
			if (msg->response.iMfwRet != 0)
			{
				callback_setStringBatch_exception(msg->response.iMfwRet);
				return;
			}
			decodeResponse_setStringBatch(msg->response.sRspPayload, this);
			return;
		}
	case 7:
		{
			if (msg->response.iMfwRet != 0)
			{
				callback_delStringBatch_exception(msg->response.iMfwRet);
				return;
			}
			decodeResponse_delStringBatch(msg->response.sRspPayload, this);
			return;
		}
	case 8:
		{
			if (msg->response.iMfwRet != 0)
			{
				callback_incr_exception(msg->response.iMfwRet);
				return;
			}

			int64_t iValue;
			decodeResponse_incr(msg->response.sRspPayload, iValue, this);
			return;
		}
	case 9:
		{
			if (msg->response.iMfwRet != 0)
			{
				callback_decr_exception(msg->response.iMfwRet);
				return;
			}

			int64_t iValue;
			decodeResponse_decr(msg->response.sRspPayload, iValue, this);
			return;
		}
	}
	throw std::runtime_error("proxy dispatch no such func");
}

int32_t RedisProxy::call(const vector<string> &vCmdArg, string &sResult)
{
	CRedisProtocolPacker packer(1);
	packer.appendCmdArg(vCmdArg);

	mfw::ResponsePacket rsp;
	rpc_call(packer.getRequestId(), "call", packer.getData(), rsp);
	swap(rsp.sRspPayload, sResult);
	return 0;
}

void RedisProxy::async_call(const RedisPrxCallbackPtr &callback, const vector<string> &vCmdArg)
{
	CRedisProtocolPacker packer(1);
	packer.appendCmdArg(vCmdArg);

	rpc_call_async(packer.getRequestId(), "call", packer.getData(), callback);
}

int32_t RedisProxy::callBatch(const vector<vector<string> > &vAllCmdArg, vector<string> &vResult)
{
	if (vAllCmdArg.empty())
	{
		return 0;
	}

	CRedisProtocolPacker packer(vAllCmdArg.size());
	packer.appendCmdArg(vAllCmdArg);

	mfw::ResponsePacket rsp;
	rpc_call(packer.getRequestId(), "callBatch", packer.getData(), rsp);

	return decodeResponse_callBatch(rsp.sRspPayload, vResult);
}

void RedisProxy::async_callBatch(const RedisPrxCallbackPtr &callback, const vector<vector<string> > &vAllCmdArg)
{
	if (vAllCmdArg.empty())
	{
		return;
	}

	CRedisProtocolPacker packer(vAllCmdArg.size());
	packer.appendCmdArg(vAllCmdArg);

	rpc_call_async(packer.getRequestId(), "callBatch", packer.getData(), callback);
}

int32_t RedisProxy::getString(const string &sKey, string &sValue)
{
	CRedisProtocolPacker packer;
	encodeRequest_getString(packer, sKey);

	mfw::ResponsePacket rsp;
	rpc_call(packer.getRequestId(), "getString", packer.getData(), rsp);

	return decodeResponse_getString(rsp.sRspPayload, sValue);
}

void RedisProxy::async_getString(const RedisPrxCallbackPtr &callback, const string &sKey)
{
	CRedisProtocolPacker packer;
	encodeRequest_getString(packer, sKey);

	rpc_call_async(packer.getRequestId(), "getString", packer.getData(), callback);
}

int32_t RedisProxy::setString(const string &sKey, const string &sValue)
{
	CRedisProtocolPacker packer;
	encodeRequest_setString(packer, sKey, sValue);

	mfw::ResponsePacket rsp;
	rpc_call(packer.getRequestId(), "setString", packer.getData(), rsp);

	return decodeResponse_setString(rsp.sRspPayload);
}

void RedisProxy::async_setString(const RedisPrxCallbackPtr &callback, const string &sKey, const string &sValue)
{
	CRedisProtocolPacker packer;
	encodeRequest_setString(packer, sKey, sValue);

	rpc_call_async(packer.getRequestId(), "setString", packer.getData(), callback);
}

int32_t RedisProxy::delString(const string &sKey)
{
	CRedisProtocolPacker packer;
	encodeRequest_delString(packer, sKey);

	mfw::ResponsePacket rsp;
	rpc_call(packer.getRequestId(), "delString", packer.getData(), rsp);

	return decodeResponse_delString(rsp.sRspPayload);
}

void RedisProxy::async_delString(const RedisPrxCallbackPtr &callback, const string &sKey)
{
	CRedisProtocolPacker packer;
	encodeRequest_delString(packer, sKey);

	rpc_call_async(packer.getRequestId(), "delString", packer.getData(), callback);
}

int32_t RedisProxy::getStringBatch(const vector<string> &vKey, map<string, string> &mKeyValue)
{
	if (vKey.empty())
	{
		return 0;
	}

	CRedisProtocolPacker packer;
	encodeRequest_getStringBatch(packer, vKey);

	mfw::ResponsePacket rsp;
	rpc_call(packer.getRequestId(), "getStringBatch", packer.getData(), rsp);

	return decodeResponse_getStringBatch(rsp.sRspPayload, mKeyValue);
}

void RedisProxy::async_getStringBatch(const RedisPrxCallbackPtr &callback, const vector<string> &vKey)
{
	if (vKey.empty())
	{
		return;
	}

	CRedisProtocolPacker packer;
	encodeRequest_getStringBatch(packer, vKey);

	rpc_call_async(packer.getRequestId(), "getStringBatch", packer.getData(), callback);
}

int32_t RedisProxy::setStringBatch(const map<string, string> &mSetValue)
{
	if (mSetValue.empty())
	{
		return 0;
	}

	CRedisProtocolPacker packer;
	encodeRequest_setStringBatch(packer, mSetValue);

	mfw::ResponsePacket rsp;
	rpc_call(packer.getRequestId(), "setStringBatch", packer.getData(), rsp);

	return decodeResponse_setStringBatch(rsp.sRspPayload);
}

void RedisProxy::async_setStringBatch(const RedisPrxCallbackPtr &callback, const map<string, string> &mSetValue)
{
	if (mSetValue.empty())
	{
		return;
	}

	CRedisProtocolPacker packer;
	encodeRequest_setStringBatch(packer, mSetValue);

	rpc_call_async(packer.getRequestId(), "setStringBatch", packer.getData(), callback);
}

int32_t RedisProxy::delStringBatch(const vector<string> &vDelKey)
{
	if (vDelKey.empty())
	{
		return 0;
	}

	CRedisProtocolPacker packer;
	encodeRequest_delStringBatch(packer, vDelKey);

	mfw::ResponsePacket rsp;
	rpc_call(packer.getRequestId(), "delStringBatch", packer.getData(), rsp);

	return decodeResponse_delStringBatch(rsp.sRspPayload);
}

void RedisProxy::async_delStringBatch(const RedisPrxCallbackPtr &callback, const vector<string> &vDelKey)
{
	if (vDelKey.empty())
	{
		return;
	}

	CRedisProtocolPacker packer;
	encodeRequest_delStringBatch(packer, vDelKey);

	rpc_call_async(packer.getRequestId(), "delStringBatch", packer.getData(), callback);
}

int32_t RedisProxy::incr(const string &sKey, int64_t iNum, int64_t &iValue)
{
	CRedisProtocolPacker packer;
	encodeRequest_incr(packer, sKey, iNum);

	mfw::ResponsePacket rsp;
	rpc_call(packer.getRequestId(), "incr", packer.getData(), rsp);

	return decodeResponse_incr(rsp.sRspPayload, iValue);
}

void RedisProxy::async_incr(const RedisPrxCallbackPtr &callback, const string &sKey, int64_t iNum)
{
	CRedisProtocolPacker packer;
	encodeRequest_incr(packer, sKey, iNum);

	rpc_call_async(packer.getRequestId(), "incr", packer.getData(), callback);
}

int32_t RedisProxy::decr(const string &sKey, int64_t iNum, int64_t &iValue)
{
	CRedisProtocolPacker packer;
	encodeRequest_decr(packer, sKey, iNum);

	mfw::ResponsePacket rsp;
	rpc_call(packer.getRequestId(), "decr", packer.getData(), rsp);

	return decodeResponse_decr(rsp.sRspPayload, iValue);
}

void RedisProxy::async_decr(const RedisPrxCallbackPtr &callback, const string &sKey, int64_t iNum)
{
	CRedisProtocolPacker packer;
	encodeRequest_decr(packer, sKey, iNum);

	rpc_call_async(packer.getRequestId(), "decr", packer.getData(), callback);
}

}
