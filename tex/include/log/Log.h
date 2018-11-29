#ifndef _SDP_LOG_HPP_
#define _SDP_LOG_HPP_

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
struct LogBaseInfo
{
	string sAppName;
	string sServerName;
	string sDivision;

	LogBaseInfo() {}
	const char *getName() const { return "LogBaseInfo"; } 
	template <typename T>
	void visit(T &t, bool bOpt) 
	{
		if (!bOpt)
		{
			if (!sAppName.empty()) t.visit(0, false, "sAppName", sAppName);
			if (!sServerName.empty()) t.visit(1, false, "sServerName", sServerName);
			if (!sDivision.empty()) t.visit(2, false, "sDivision", sDivision);
		}
		else
		{
			t.visit(0, false, "sAppName", sAppName);
			t.visit(1, false, "sServerName", sServerName);
			t.visit(2, false, "sDivision", sDivision);
		}
	}
	template <typename T>
	void visit(T &t, bool bOpt) const 
	{
		if (!bOpt)
		{
			if (!sAppName.empty()) t.visit(0, false, "sAppName", sAppName);
			if (!sServerName.empty()) t.visit(1, false, "sServerName", sServerName);
			if (!sDivision.empty()) t.visit(2, false, "sDivision", sDivision);
		}
		else
		{
			t.visit(0, false, "sAppName", sAppName);
			t.visit(1, false, "sServerName", sServerName);
			t.visit(2, false, "sDivision", sDivision);
		}
	}
	void swap(LogBaseInfo &b)
	{
		using std::swap;
		swap(sAppName, b.sAppName);
		swap(sServerName, b.sServerName);
		swap(sDivision, b.sDivision);
	}
	bool operator== (const LogBaseInfo &rhs) const
	{
		return sAppName == rhs.sAppName
			&& sServerName == rhs.sServerName
			&& sDivision == rhs.sDivision;
	}
	bool operator!= (const LogBaseInfo &rhs) const
	{
		return !((*this) == rhs);
	}
};

struct LogDataItem
{
	uint32_t iTime;
	string sData;

	LogDataItem(): 
		iTime(0)
	{
	}
	const char *getName() const { return "LogDataItem"; } 
	template <typename T>
	void visit(T &t, bool bOpt) 
	{
		if (!bOpt)
		{
			if (iTime != 0) t.visit(0, false, "iTime", iTime);
			if (!sData.empty()) t.visit(1, false, "sData", sData);
		}
		else
		{
			t.visit(0, false, "iTime", iTime);
			t.visit(1, false, "sData", sData);
		}
	}
	template <typename T>
	void visit(T &t, bool bOpt) const 
	{
		if (!bOpt)
		{
			if (iTime != 0) t.visit(0, false, "iTime", iTime);
			if (!sData.empty()) t.visit(1, false, "sData", sData);
		}
		else
		{
			t.visit(0, false, "iTime", iTime);
			t.visit(1, false, "sData", sData);
		}
	}
	void swap(LogDataItem &b)
	{
		using std::swap;
		swap(iTime, b.iTime);
		swap(sData, b.sData);
	}
	bool operator== (const LogDataItem &rhs) const
	{
		return iTime == rhs.iTime
			&& sData == rhs.sData;
	}
	bool operator!= (const LogDataItem &rhs) const
	{
		return !((*this) == rhs);
	}
};

class LogPrxCallback: public mfw::ServiceProxyCallback
{
public:
	tr1::shared_ptr<LogPrxCallback> shared_from_this() { return tr1::static_pointer_cast<LogPrxCallback>(tr1::enable_shared_from_this<ServiceProxyCallback>::shared_from_this()); }

	virtual void callback_logRemote(int32_t /*ret*/)
	{ throw std::runtime_error("callback_logRemote() overloading incorrect."); }
	virtual void callback_logRemote_exception(int32_t /*ret*/)
	{ throw std::runtime_error("callback_logRemote_exception() overloading incorrect."); }

	virtual void onDispatch(mfw::ReqMessage *msg);
};
typedef tr1::shared_ptr<LogPrxCallback> LogPrxCallbackPtr;

class LogProxy : public mfw::ServiceProxy
{
public:
	int32_t logRemote(const mfw::LogBaseInfo &info, const map<string, vector<mfw::LogDataItem> > &mFileData);
	void async_logRemote(const LogPrxCallbackPtr &callback, const mfw::LogBaseInfo &info, const map<string, vector<mfw::LogDataItem> > &mFileData);
};
typedef tr1::shared_ptr<LogProxy> LogPrx;

class Log : public mfw::Service
{
public:
	virtual int handleMfwRequest(string &sRspPayload);
	virtual int32_t logRemote(const mfw::LogBaseInfo &info, const map<string, vector<mfw::LogDataItem> > &mFileData) = 0;
	static void async_response_logRemote(const mfw::SdpCurrentPtr &current, int32_t ret);
};

}

namespace std
{
inline void swap(mfw::LogBaseInfo &a, mfw::LogBaseInfo &b) { a.swap(b); }
inline void swap(mfw::LogDataItem &a, mfw::LogDataItem &b) { a.swap(b); }
}

#endif
