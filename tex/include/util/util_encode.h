#ifndef _MFW_UTIL_ENCODE_
#define _MFW_UTIL_ENCODE_

#include <stdint.h>
#include <string>
using namespace std;

namespace mfw
{

class UtilEncode
{
public:
    static string toHex(const string &s);
    static string fromHex(const string &s);

    static string c_unescape(const string &s);
    static string c_escape(const string &s);

    static string urlencode(const string &sUrl); // 空格转义+,~转义%7e
    static string urldecode(const string &sUrl);
    static string rawurlencode(const string &sUrl); // 空格转义%20,~不转义
    static string rawurldecode(const string &sUrl);
};

}

#endif
