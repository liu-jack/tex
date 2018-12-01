#ifndef _MFW_UTIL_BASE64_
#define _MFW_UTIL_BASE64_

#include <stdint.h>
#include <string>
using namespace std;

namespace mfw
{

class UtilBase64
{
public:
    static uint32_t getEncodeLength(uint32_t iLen)
    {
        return (iLen + 2) / 3 * 4;
    }

    static string encode(const string &sData);
    static string decode(const string &sData);

    static void encode(const string &sData, string &sResult);
    static bool decode(const string &sData, string &sResult);

    static void encode(const void *pInBuf, uint32_t iInLen, void *pOutBuf, uint32_t iOutLen);
    static bool decode(const void *pInBuf, uint32_t iInLen, void *pOutBuf, uint32_t &iOutLen);
};

}

#endif
