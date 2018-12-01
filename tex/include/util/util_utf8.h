#ifndef _MFW_UTIL_UTF8_
#define _MFW_UTIL_UTF8_

#include <stdint.h>
#include <string>
using namespace std;

namespace mfw
{

class UtilUTF8
{
public:
    static bool isValidUTF8(const string &s);
    static uint32_t utf8Length(const string &s);
    static string utf8UpperCase(const string &s);
    static string utf8LowerCase(const string &s);
    static string utf8Substr(const string &s, uint32_t pos, uint32_t len);
};

}

#endif
