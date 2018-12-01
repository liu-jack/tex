#include "util_http.h"
#include "util_string.h"
#include "util_encode.h"
#include  <curl/curl.h>
#include <stdexcept>
#include <assert.h>

#define CHECK_CURL(code) if (code != CURLE_OK) \
						{\
							throw std::runtime_error("invalid curl option:" + UtilString::tostr(curl_easy_strerror(code)));\
						}

namespace mfw
{
static size_t callback_write(void* buffer, size_t size, size_t nmemb, void* lpVoid)
{
    string* str = dynamic_cast<std::string*>((string *)lpVoid);
    if( NULL == str || NULL == buffer ) {
        return -1;
    }

    char* pData = (char*)buffer;
    str->append(pData, size * nmemb);
    return nmemb;
}

static size_t callback_header(char *buffer, size_t size, size_t nitems, void *lpVoid)
{
    string* str = dynamic_cast<std::string*>((string *)lpVoid);
    if( NULL == str || NULL == buffer ) {
        return -1;
    }

    char* pData = (char*)buffer;
    str->append(pData, nitems * size);
    return nitems * size;
}

void UtilHttp::fetchURL(const string &sUrl, ResponseData &stResponseData, const RequestOption &stOption)
{
    CURLcode ret;
    CURL* pCurl = curl_easy_init();
    if (pCurl == NULL) {
        throw std::runtime_error("init curl failed");
    }

    ret = curl_easy_setopt(pCurl, CURLOPT_URL, sUrl.c_str());
    CHECK_CURL(ret);

    ret = curl_easy_setopt(pCurl, CURLOPT_CONNECTTIMEOUT_MS, stOption.iConnectTimeoutMS);
    CHECK_CURL(ret);

    ret = curl_easy_setopt(pCurl, CURLOPT_TIMEOUT_MS, stOption.iTimeoutMS);
    CHECK_CURL(ret);

    ret = curl_easy_setopt(pCurl, CURLOPT_NOSIGNAL, 1L);
    CHECK_CURL(ret);

    ret = curl_easy_setopt(pCurl, CURLOPT_READFUNCTION, NULL);
    CHECK_CURL(ret);

    ret = curl_easy_setopt(pCurl, CURLOPT_WRITEFUNCTION, callback_write);
    CHECK_CURL(ret);

    ret = curl_easy_setopt(pCurl, CURLOPT_WRITEDATA, (void *)&stResponseData.sResponse);
    CHECK_CURL(ret);

    ret = curl_easy_setopt(pCurl, CURLOPT_HEADERFUNCTION, callback_header);
    CHECK_CURL(ret);

    ret = curl_easy_setopt(pCurl, CURLOPT_HEADERDATA, (void *)&stResponseData.sHeader);
    CHECK_CURL(ret);

    if (stOption.bVerbose) {
        ret = curl_easy_setopt(pCurl, CURLOPT_VERBOSE, 1L);
        CHECK_CURL(ret);
    }

    if (stOption.bPost) {
        ret = curl_easy_setopt(pCurl, CURLOPT_POST, 1L);
        CHECK_CURL(ret);

        ret = curl_easy_setopt(pCurl, CURLOPT_POSTFIELDS, stOption.sPostFields.c_str());
        CHECK_CURL(ret);
    }

    struct curl_slist *chunk = NULL;
    map<string,string>::const_iterator iter = stOption.mHeader.begin();
    while (iter != stOption.mHeader.end()) {
        string sHeader = UtilString::format("%s: %s", iter->first.c_str(), iter->second.c_str());
        chunk = curl_slist_append(chunk, sHeader.c_str());
        ++iter;
    }
    ret = curl_easy_setopt(pCurl, CURLOPT_HTTPHEADER, chunk);
    CHECK_CURL(ret);

    ret = curl_easy_perform(pCurl);
    if (ret != CURLE_OK) {
        throw std::runtime_error("curl perform error:"+UtilString::tostr(curl_easy_strerror(ret)));
    }

    vector<string> vHeader = UtilString::splitString(stResponseData.sHeader, "\r\n");
    if (vHeader.size() > 1) {
        vector<string> mCode = UtilString::splitString(vHeader[0], " ");
        stResponseData.iCode = UtilString::strto<uint32_t>(mCode[1]);

        for (uint32_t i = 1; i < vHeader.size(); ++i) {
            string::size_type p = vHeader[i].find_first_of(":");
            if (p != string::npos) {
                stResponseData.mHeader[vHeader[i].substr(0,p)] = vHeader[i].substr(p+2);
            }
        }
    }

    curl_slist_free_all(chunk);
    curl_easy_cleanup(pCurl);
}

UtilHttp::Requestor::~Requestor()
{
    cleanup();
}

void UtilHttp::Requestor::init()
{
    if (m_curl != NULL) return;

    m_curl = curl_easy_init();
    if (m_curl == NULL) {
        throw std::runtime_error("init curl failed");
    }
}

void UtilHttp::Requestor::reset()
{
    if (m_curl == NULL) return;

    curl_easy_reset(m_curl);
}

void UtilHttp::Requestor::cleanup()
{
    if (m_curl == NULL) return;

    curl_easy_cleanup(m_curl);
    m_curl = NULL;
}

void UtilHttp::Requestor::perform(const string &sUrl, ResponseData &stResponseData, const RequestOption &stOption)
{
    init();

    CURLcode ret;
    ret = curl_easy_setopt(m_curl, CURLOPT_URL, sUrl.c_str());
    CHECK_CURL(ret);

    ret = curl_easy_setopt(m_curl, CURLOPT_CONNECTTIMEOUT_MS, stOption.iConnectTimeoutMS);
    CHECK_CURL(ret);

    ret = curl_easy_setopt(m_curl, CURLOPT_TIMEOUT_MS, stOption.iTimeoutMS);
    CHECK_CURL(ret);

    if (!stOption.bSSLVerifyHost) {
        ret = curl_easy_setopt(m_curl, CURLOPT_SSL_VERIFYHOST, 0L);
        CHECK_CURL(ret);
    }

    if (!stOption.bSSLVerifyHost) {
        ret = curl_easy_setopt(m_curl, CURLOPT_SSL_VERIFYPEER, 0L);
        CHECK_CURL(ret);
    }

    ret = curl_easy_setopt(m_curl, CURLOPT_NOSIGNAL, 1L);
    CHECK_CURL(ret);

    ret = curl_easy_setopt(m_curl, CURLOPT_READFUNCTION, NULL);
    CHECK_CURL(ret);

    ret = curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, callback_write);
    CHECK_CURL(ret);

    ret = curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, (void *)&stResponseData.sResponse);
    CHECK_CURL(ret);

    ret = curl_easy_setopt(m_curl, CURLOPT_HEADERFUNCTION, callback_header);
    CHECK_CURL(ret);

    ret = curl_easy_setopt(m_curl, CURLOPT_HEADERDATA, (void *)&stResponseData.sHeader);
    CHECK_CURL(ret);

    if (stOption.bVerbose) {
        ret = curl_easy_setopt(m_curl, CURLOPT_VERBOSE, 1L);
        CHECK_CURL(ret);
    }

    if (stOption.bPost) {
        ret = curl_easy_setopt(m_curl, CURLOPT_POST, 1L);
        CHECK_CURL(ret);

        ret = curl_easy_setopt(m_curl, CURLOPT_POSTFIELDS, stOption.sPostFields.c_str());
        CHECK_CURL(ret);
    }

    struct curl_slist *chunk = NULL;
    map<string,string>::const_iterator iter = stOption.mHeader.begin();
    while (iter != stOption.mHeader.end()) {
        string sHeader = UtilString::format("%s: %s", iter->first.c_str(), iter->second.c_str());
        chunk = curl_slist_append(chunk, sHeader.c_str());
        ++iter;
    }
    ret = curl_easy_setopt(m_curl, CURLOPT_HTTPHEADER, chunk);
    CHECK_CURL(ret);

    ret = curl_easy_perform(m_curl);
    if (ret != CURLE_OK) {
        throw std::runtime_error("curl perform error:"+UtilString::tostr(curl_easy_strerror(ret)));
    }

    vector<string> vHeader = UtilString::splitString(stResponseData.sHeader, "\r\n");
    if (vHeader.size() > 1) {
        vector<string> mCode = UtilString::splitString(vHeader[0], " ");
        stResponseData.iCode = UtilString::strto<uint32_t>(mCode[1]);

        for (uint32_t i = 1; i < vHeader.size(); ++i) {
            string::size_type p = vHeader[i].find_first_of(":");
            if (p != string::npos) {
                stResponseData.mHeader[vHeader[i].substr(0,p)] = vHeader[i].substr(p+2);
            }
        }
    }

    curl_slist_free_all(chunk);

    reset();
}

UtilHttp::RequestorPtr UtilHttp::RequestorPool::get()
{
    CLockGuard<CMutex> lock(m_mutex);
    if (!m_pool.empty()) {
        RequestorPtr requestor = m_pool.front();
        m_pool.pop_front();
        return requestor;
    }
    lock.unlock();

    RequestorPtr requestor(new Requestor);
    return requestor;
}

void UtilHttp::RequestorPool::put(const RequestorPtr &requestor)
{
    CLockGuard<CMutex> lock(m_mutex);
    if (m_pool.size() >= m_iMaxNum) return;
    m_pool.push_back(requestor);
}

void UtilHttp::RequestorPool::setMaxNum(uint32_t iMaxNum)
{
    CLockGuard<CMutex> lock(m_mutex);
    m_iMaxNum = iMaxNum;

    while (m_pool.size() > m_iMaxNum) {
        m_pool.pop_front();
    }
}

string UtilHttp::appendURLParam(const string &sURL, const map<string, string> &mParam)
{
    string sEncodeURL = sURL;
    sEncodeURL += "?";
    sEncodeURL += UtilString::joinURLParam(mParam);
    return sEncodeURL;
}

void UtilHttp::extractURLParam(const string &sURL, string &sLocation, map<string, string> &mParam)
{
    if (sURL.empty()) return;

    vector<string> vURL = UtilString::splitString(sURL, "?");

    sLocation = vURL[0];
    if (vURL.size() > 1) {
        UtilString::splitURLParam(vURL[1], mParam);
    }
}
}
