#ifndef _UTIL_HTTP_H_
#define _UTIL_HTTP_H_

#include <map>
#include <string>
#include <stdint.h>
#include <deque>
#include "util_thread.h"

using namespace std;

namespace mfw
{

class UtilHttp
{
public:
	struct RequestOption
	{
		bool            bVerbose;
		bool            bPost;
        bool            bSSLVerifyHost;
        bool            bSSLVerifyPeer;
		string          sPostFields;
		uint32_t        iTimeoutMS;
		uint32_t        iConnectTimeoutMS;
		map<string, string> mHeader;

		RequestOption():
        	bVerbose(false), bPost(false), bSSLVerifyHost(false), bSSLVerifyPeer(false), iTimeoutMS(5000), iConnectTimeoutMS(3000)
        {}
	};

	struct ResponseData
	{
		uint32_t        iCode;
        string          sHeader;
		map<string, string> mHeader;
		string          sResponse;
	};

	class Requestor
	{
	public:
		Requestor() : m_curl(NULL) {}
		~Requestor();

		void perform(const string &sURL, ResponseData &stResponseData, const RequestOption &stOption = RequestOption());

	private:
		void init();
		void reset();
		void cleanup();

	private:
		void			*m_curl;
	};

	typedef tr1::shared_ptr<Requestor> RequestorPtr;

	class RequestorPool
	{
	public:
		RequestorPool() : m_iMaxNum(0) {}
		explicit RequestorPool(uint32_t iMaxNum) : m_iMaxNum(iMaxNum) {}

		RequestorPtr get();
		void put(const RequestorPtr &requestor);
		void setMaxNum(uint32_t iMaxNum);

	private:
		CMutex				m_mutex;
		uint32_t			m_iMaxNum;
		deque<RequestorPtr>	m_pool;
	};

	static void fetchURL(const string &sURL, ResponseData &stResposeData, const RequestOption &stOption = RequestOption());

	static string appendURLParam(const string &sURL, const map<string, string> &mParam);
	static void extractURLParam(const string &sURL, string &sLocation, map<string, string> &mParam);
};

}
#endif
