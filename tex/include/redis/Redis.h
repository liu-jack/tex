#ifndef _SDP_REDIS_HPP_
#define _SDP_REDIS_HPP_

#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include "sdp/Sdp.h"
#include "service/Service.h"
using namespace std;
using namespace mfw;

namespace mfw
{
enum RedisResult
{
	RedisResult_NoData = -999980,
};
static inline const char *etos(RedisResult e)
{
	switch (e)
	{
		case RedisResult_NoData: return "RedisResult_NoData";
		default: return "";
	}
}
static inline bool stoe(const string &s, RedisResult &e)
{
	if (s == "RedisResult_NoData") { e = RedisResult_NoData; return true; }
	return false;
}

class RedisPrxCallback: public mfw::ServiceProxyCallback
{
public:
	tr1::shared_ptr<RedisPrxCallback> shared_from_this() { return tr1::static_pointer_cast<RedisPrxCallback>(tr1::enable_shared_from_this<ServiceProxyCallback>::shared_from_this()); }

	virtual void callback_call(int32_t /*ret*/, const string &/*sResult*/)
	{ throw std::runtime_error("callback_call() overloading incorrect."); }
	virtual void callback_call_exception(int32_t /*ret*/)
	{ throw std::runtime_error("callback_call_exception() overloading incorrect."); }

	virtual void callback_callBatch(int32_t /*ret*/, const vector<string> &/*vResult*/)
	{ throw std::runtime_error("callback_callBatch() overloading incorrect."); }
	virtual void callback_callBatch_exception(int32_t /*ret*/)
	{ throw std::runtime_error("callback_callBatch_exception() overloading incorrect."); }

	virtual void callback_getString(int32_t /*ret*/, const string &/*sValue*/)
	{ throw std::runtime_error("callback_getString() overloading incorrect."); }
	virtual void callback_getString_exception(int32_t /*ret*/)
	{ throw std::runtime_error("callback_getString_exception() overloading incorrect."); }

	virtual void callback_setString(int32_t /*ret*/)
	{ throw std::runtime_error("callback_setString() overloading incorrect."); }
	virtual void callback_setString_exception(int32_t /*ret*/)
	{ throw std::runtime_error("callback_setString_exception() overloading incorrect."); }

	virtual void callback_delString(int32_t /*ret*/)
	{ throw std::runtime_error("callback_delString() overloading incorrect."); }
	virtual void callback_delString_exception(int32_t /*ret*/)
	{ throw std::runtime_error("callback_delString_exception() overloading incorrect."); }

	virtual void callback_getStringBatch(int32_t /*ret*/, const map<string, string> &/*mKeyValue*/)
	{ throw std::runtime_error("callback_getStringBatch() overloading incorrect."); }
	virtual void callback_getStringBatch_exception(int32_t /*ret*/)
	{ throw std::runtime_error("callback_getStringBatch_exception() overloading incorrect."); }

	virtual void callback_setStringBatch(int32_t /*ret*/)
	{ throw std::runtime_error("callback_setStringBatch() overloading incorrect."); }
	virtual void callback_setStringBatch_exception(int32_t /*ret*/)
	{ throw std::runtime_error("callback_setStringBatch_exception() overloading incorrect."); }

	virtual void callback_delStringBatch(int32_t /*ret*/)
	{ throw std::runtime_error("callback_delStringBatch() overloading incorrect."); }
	virtual void callback_delStringBatch_exception(int32_t /*ret*/)
	{ throw std::runtime_error("callback_delStringBatch_exception() overloading incorrect."); }

	virtual void callback_incr(int32_t /*ret*/, int64_t /*iValue*/)
	{ throw std::runtime_error("callback_incr() overloading incorrect."); }
	virtual void callback_incr_exception(int32_t /*ret*/)
	{ throw std::runtime_error("callback_incr_exception() overloading incorrect."); }

	virtual void callback_decr(int32_t /*ret*/, int64_t /*iValue*/)
	{ throw std::runtime_error("callback_decr() overloading incorrect."); }
	virtual void callback_decr_exception(int32_t /*ret*/)
	{ throw std::runtime_error("callback_decr_exception() overloading incorrect."); }

	virtual void onDispatch(mfw::ReqMessage *msg);
};
typedef tr1::shared_ptr<RedisPrxCallback> RedisPrxCallbackPtr;

class RedisProxy : public mfw::ServiceProxy
{
public:
	void initRedisProtocol();
	int32_t call(const vector<string> &vCmdArg, string &sResult);
	void async_call(const RedisPrxCallbackPtr &callback, const vector<string> &vCmdArg);
	int32_t callBatch(const vector<vector<string> > &vAllCmdArg, vector<string> &vResult);
	void async_callBatch(const RedisPrxCallbackPtr &callback, const vector<vector<string> > &vAllCmdArg);
	int32_t getString(const string &sKey, string &sValue);
	void async_getString(const RedisPrxCallbackPtr &callback, const string &sKey);
	int32_t setString(const string &sKey, const string &sValue);
	void async_setString(const RedisPrxCallbackPtr &callback, const string &sKey, const string &sValue);
	int32_t delString(const string &sKey);
	void async_delString(const RedisPrxCallbackPtr &callback, const string &sKey);
	int32_t getStringBatch(const vector<string> &vKey, map<string, string> &mKeyValue);
	void async_getStringBatch(const RedisPrxCallbackPtr &callback, const vector<string> &vKey);
	int32_t setStringBatch(const map<string, string> &mSetValue);
	void async_setStringBatch(const RedisPrxCallbackPtr &callback, const map<string, string> &mSetValue);
	int32_t delStringBatch(const vector<string> &vDelKey);
	void async_delStringBatch(const RedisPrxCallbackPtr &callback, const vector<string> &vDelKey);
	int32_t incr(const string &sKey, int64_t iNum, int64_t &iValue);
	void async_incr(const RedisPrxCallbackPtr &callback, const string &sKey, int64_t iNum);
	int32_t decr(const string &sKey, int64_t iNum, int64_t &iValue);
	void async_decr(const RedisPrxCallbackPtr &callback, const string &sKey, int64_t iNum);
};
typedef tr1::shared_ptr<RedisProxy> RedisPrx;

}


#endif
