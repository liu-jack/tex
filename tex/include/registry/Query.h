#ifndef _SDP_QUERY_HPP_
#define _SDP_QUERY_HPP_

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
class QueryPrxCallback: public mfw::ServiceProxyCallback
{
public:
	tr1::shared_ptr<QueryPrxCallback> shared_from_this() { return tr1::static_pointer_cast<QueryPrxCallback>(tr1::enable_shared_from_this<ServiceProxyCallback>::shared_from_this()); }

	virtual void callback_getEndpoints(int32_t /*ret*/, const vector<string> &/*vActiveEps*/, const vector<string> &/*vInactiveEps*/)
	{ throw std::runtime_error("callback_getEndpoints() overloading incorrect."); }
	virtual void callback_getEndpoints_exception(int32_t /*ret*/)
	{ throw std::runtime_error("callback_getEndpoints_exception() overloading incorrect."); }

	virtual void callback_addEndpoint(int32_t /*ret*/)
	{ throw std::runtime_error("callback_addEndpoint() overloading incorrect."); }
	virtual void callback_addEndpoint_exception(int32_t /*ret*/)
	{ throw std::runtime_error("callback_addEndpoint_exception() overloading incorrect."); }

	virtual void callback_removeEndpoint(int32_t /*ret*/)
	{ throw std::runtime_error("callback_removeEndpoint() overloading incorrect."); }
	virtual void callback_removeEndpoint_exception(int32_t /*ret*/)
	{ throw std::runtime_error("callback_removeEndpoint_exception() overloading incorrect."); }

	virtual void onDispatch(mfw::ReqMessage *msg);
};
typedef tr1::shared_ptr<QueryPrxCallback> QueryPrxCallbackPtr;

class QueryProxy : public mfw::ServiceProxy
{
public:
	int32_t getEndpoints(const string &sObj, const string &sDivision, vector<string> &vActiveEps, vector<string> &vInactiveEps);
	void async_getEndpoints(const QueryPrxCallbackPtr &callback, const string &sObj, const string &sDivision);
	int32_t addEndpoint(const string &sObj, const string &sDivision, const string &ep);
	void async_addEndpoint(const QueryPrxCallbackPtr &callback, const string &sObj, const string &sDivision, const string &ep);
	int32_t removeEndpoint(const string &sObj, const string &sDivision, const string &ep);
	void async_removeEndpoint(const QueryPrxCallbackPtr &callback, const string &sObj, const string &sDivision, const string &ep);
};
typedef tr1::shared_ptr<QueryProxy> QueryPrx;

class Query : public mfw::Service
{
public:
	virtual int handleMfwRequest(string &sRspPayload);
	virtual int32_t getEndpoints(const string &sObj, const string &sDivision, vector<string> &vActiveEps, vector<string> &vInactiveEps) = 0;
	virtual int32_t addEndpoint(const string &sObj, const string &sDivision, const string &ep) = 0;
	virtual int32_t removeEndpoint(const string &sObj, const string &sDivision, const string &ep) = 0;
	static void async_response_getEndpoints(const mfw::SdpCurrentPtr &current, int32_t ret, const vector<string> &vActiveEps, const vector<string> &vInactiveEps);
	static void async_response_addEndpoint(const mfw::SdpCurrentPtr &current, int32_t ret);
	static void async_response_removeEndpoint(const mfw::SdpCurrentPtr &current, int32_t ret);
};

}


#endif
