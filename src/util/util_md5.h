#ifndef _MFW_UTIL_MD5_
#define _MFW_UTIL_MD5_

#include <stdint.h>
#include <string>
using namespace std;

namespace mfw
{

class UtilMD5
{
public:
    // 结果：16字节二进制数据
    static string md5bin(const string &s);
    static string md5filebin(const string &sFile);

    // 结果：32字节HEX字符串
    static string md5hex(const string &s);
    static string md5filehex(const string &sFile);
};

}
#endif
