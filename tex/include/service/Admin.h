#ifndef _SDP_ADMIN_HPP_
#define _SDP_ADMIN_HPP_

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
class AdminPrxCallback: public mfw::ServiceProxyCallback
{
public:
	tr1::shared_ptr<AdminPrxCallback> shared_from_this() { return tr1::static_pointer_cast<AdminPrxCallback>(tr1::enable_shared_from_this<ServiceProxyCallback>::shared_from_this()); }

	virtual void callback_shutdown()
	{ throw std::runtime_error("callback_shutdown() overloading incorrect."); }
	virtual void callback_shutdown_exception(int32_t /*ret*/)
	{ throw std::runtime_error("callback_shutdown_exception() overloading incorrect."); }

	virtual void callback_notify(const string &/*ret*/)
	{ throw std::runtime_error("callback_notify() overloading incorrect."); }
	virtual void callback_notify_exception(int32_t /*ret*/)
	{ throw std::runtime_error("callback_notify_exception() overloading incorrect."); }

	virtual void onDispatch(mfw::ReqMessage *msg);
};
typedef tr1::shared_ptr<AdminPrxCallback> AdminPrxCallbackPtr;

class AdminProxy : public mfw::ServiceProxy
{
public:
	void shutdown();
	void async_shutdown(const AdminPrxCallbackPtr &callback);
	string notify(const string &command);
	void async_notify(const AdminPrxCallbackPtr &callback, const string &command);
};
typedef tr1::shared_ptr<AdminProxy> AdminPrx;

class Admin : public mfw::Service
{
public:
	virtual int handleMfwRequest(string &sRspPayload);
	virtual void shutdown() = 0;
	virtual string notify(const string &command) = 0;
	static void async_response_shutdown(const mfw::SdpCurrentPtr &current);
	static void async_response_notify(const mfw::SdpCurrentPtr &current, const string &ret);
};

}


#endif
