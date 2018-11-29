#ifndef _SDP_MFWPACKET_HPP_
#define _SDP_MFWPACKET_HPP_

#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include "sdp/Sdp.h"
using namespace std;
using namespace mfw;

namespace mfw
{
struct RequestPacket
{
	bool bIsOneWay;
	uint32_t iRequestId;
	string sServiceName;
	string sFuncName;
	string sReqPayload;
	uint32_t iTimeout;
	map<string, string> context;

	RequestPacket(): 
		bIsOneWay(false), 
		iRequestId(0), 
		iTimeout(0)
	{
	}
	const char *getName() const { return "RequestPacket"; } 
	template <typename T>
	void visit(T &t, bool bOpt) 
	{
		if (!bOpt)
		{
			if (bIsOneWay != false) t.visit(0, false, "bIsOneWay", bIsOneWay);
			if (iRequestId != 0) t.visit(1, false, "iRequestId", iRequestId);
			if (!sServiceName.empty()) t.visit(2, false, "sServiceName", sServiceName);
			if (!sFuncName.empty()) t.visit(3, false, "sFuncName", sFuncName);
			if (!sReqPayload.empty()) t.visit(4, false, "sReqPayload", sReqPayload);
			if (iTimeout != 0) t.visit(5, false, "iTimeout", iTimeout);
			if (!context.empty()) t.visit(6, false, "context", context);
		}
		else
		{
			t.visit(0, false, "bIsOneWay", bIsOneWay);
			t.visit(1, false, "iRequestId", iRequestId);
			t.visit(2, false, "sServiceName", sServiceName);
			t.visit(3, false, "sFuncName", sFuncName);
			t.visit(4, false, "sReqPayload", sReqPayload);
			t.visit(5, false, "iTimeout", iTimeout);
			t.visit(6, false, "context", context);
		}
	}
	template <typename T>
	void visit(T &t, bool bOpt) const 
	{
		if (!bOpt)
		{
			if (bIsOneWay != false) t.visit(0, false, "bIsOneWay", bIsOneWay);
			if (iRequestId != 0) t.visit(1, false, "iRequestId", iRequestId);
			if (!sServiceName.empty()) t.visit(2, false, "sServiceName", sServiceName);
			if (!sFuncName.empty()) t.visit(3, false, "sFuncName", sFuncName);
			if (!sReqPayload.empty()) t.visit(4, false, "sReqPayload", sReqPayload);
			if (iTimeout != 0) t.visit(5, false, "iTimeout", iTimeout);
			if (!context.empty()) t.visit(6, false, "context", context);
		}
		else
		{
			t.visit(0, false, "bIsOneWay", bIsOneWay);
			t.visit(1, false, "iRequestId", iRequestId);
			t.visit(2, false, "sServiceName", sServiceName);
			t.visit(3, false, "sFuncName", sFuncName);
			t.visit(4, false, "sReqPayload", sReqPayload);
			t.visit(5, false, "iTimeout", iTimeout);
			t.visit(6, false, "context", context);
		}
	}
	void swap(RequestPacket &b)
	{
		using std::swap;
		swap(bIsOneWay, b.bIsOneWay);
		swap(iRequestId, b.iRequestId);
		swap(sServiceName, b.sServiceName);
		swap(sFuncName, b.sFuncName);
		swap(sReqPayload, b.sReqPayload);
		swap(iTimeout, b.iTimeout);
		swap(context, b.context);
	}
	bool operator== (const RequestPacket &rhs) const
	{
		return bIsOneWay == rhs.bIsOneWay
			&& iRequestId == rhs.iRequestId
			&& sServiceName == rhs.sServiceName
			&& sFuncName == rhs.sFuncName
			&& sReqPayload == rhs.sReqPayload
			&& iTimeout == rhs.iTimeout
			&& context == rhs.context;
	}
	bool operator!= (const RequestPacket &rhs) const
	{
		return !((*this) == rhs);
	}
};

struct ResponsePacket
{
	int32_t iMfwRet;
	uint32_t iRequestId;
	string sRspPayload;
	map<string, string> context;

	ResponsePacket(): 
		iMfwRet(0), 
		iRequestId(0)
	{
	}
	const char *getName() const { return "ResponsePacket"; } 
	template <typename T>
	void visit(T &t, bool bOpt) 
	{
		if (!bOpt)
		{
			if (iMfwRet != 0) t.visit(0, false, "iMfwRet", iMfwRet);
			if (iRequestId != 0) t.visit(1, false, "iRequestId", iRequestId);
			if (!sRspPayload.empty()) t.visit(2, false, "sRspPayload", sRspPayload);
			if (!context.empty()) t.visit(3, false, "context", context);
		}
		else
		{
			t.visit(0, false, "iMfwRet", iMfwRet);
			t.visit(1, false, "iRequestId", iRequestId);
			t.visit(2, false, "sRspPayload", sRspPayload);
			t.visit(3, false, "context", context);
		}
	}
	template <typename T>
	void visit(T &t, bool bOpt) const 
	{
		if (!bOpt)
		{
			if (iMfwRet != 0) t.visit(0, false, "iMfwRet", iMfwRet);
			if (iRequestId != 0) t.visit(1, false, "iRequestId", iRequestId);
			if (!sRspPayload.empty()) t.visit(2, false, "sRspPayload", sRspPayload);
			if (!context.empty()) t.visit(3, false, "context", context);
		}
		else
		{
			t.visit(0, false, "iMfwRet", iMfwRet);
			t.visit(1, false, "iRequestId", iRequestId);
			t.visit(2, false, "sRspPayload", sRspPayload);
			t.visit(3, false, "context", context);
		}
	}
	void swap(ResponsePacket &b)
	{
		using std::swap;
		swap(iMfwRet, b.iMfwRet);
		swap(iRequestId, b.iRequestId);
		swap(sRspPayload, b.sRspPayload);
		swap(context, b.context);
	}
	bool operator== (const ResponsePacket &rhs) const
	{
		return iMfwRet == rhs.iMfwRet
			&& iRequestId == rhs.iRequestId
			&& sRspPayload == rhs.sRspPayload
			&& context == rhs.context;
	}
	bool operator!= (const ResponsePacket &rhs) const
	{
		return !((*this) == rhs);
	}
};

}

namespace std
{
inline void swap(mfw::RequestPacket &a, mfw::RequestPacket &b) { a.swap(b); }
inline void swap(mfw::ResponsePacket &a, mfw::ResponsePacket &b) { a.swap(b); }
}

#endif
