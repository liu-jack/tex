#include "util_base64.h"
#include "util_impl/base64.h"
#include <stdexcept>

namespace mfw
{

string UtilBase64::encode(const string &sData)
{
	string sResult;
	encode(sData, sResult);
	return sResult;
}

string UtilBase64::decode(const string &sData)
{
	string sResult;
	if (!decode(sData, sResult))
	{
		throw std::runtime_error("base64 decode");
	}
	return sResult;
}

void UtilBase64::encode(const string &sData, string &sResult)
{
	uint32_t iOutLen = getEncodeLength(sData.size());
	sResult.resize(iOutLen);
	encode(sData.c_str(), sData.size(), &sResult[0], iOutLen);
}

bool UtilBase64::decode(const string &sData, string &sResult)
{
	uint32_t iOutLen = 3 * (sData.size() / 4) + 2;
	string sTemp;
	sTemp.resize(iOutLen);
	if (!decode(sData.c_str(), sData.size(), &sTemp[0], iOutLen))
	{
		return false;
	}
	sTemp.resize(iOutLen);
	swap(sTemp, sResult);
	return true;
}

void UtilBase64::encode(const void *pInBuf, uint32_t iInLen, void *pOutBuf, uint32_t iOutLen)
{
	base64_encode((const char *)pInBuf, iInLen, (char *)pOutBuf, iOutLen);
}

bool UtilBase64::decode(const void *pInBuf, uint32_t iInLen, void *pOutBuf, uint32_t &iOutLen)
{
	size_t iRealLen = iOutLen;
	struct base64_decode_context ctx;
	base64_decode_ctx_init(&ctx);
	if (!base64_decode_ctx(&ctx, (const char *)pInBuf, iInLen, (char *)pOutBuf, &iRealLen))
	{
		return false;
	}
	iOutLen = iRealLen;
	return true;
}

}
