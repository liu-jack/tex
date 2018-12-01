#ifndef _MFW_UTIL_SHA_
#define _MFW_UTIL_SHA_

#include <stdint.h>
#include <string>
using namespace std;

namespace mfw
{

class UtilSHA
{
public:
    // 结果：40字节二进制数据
    static string sha1bin(const string &s);
    static string sha1filebin(const string &sFile);

    // 结果：80字节HEX字符串
    static string sha1hex(const string &s);
    static string sha1filehex(const string &sFile);

    // 结果：28字节二进制数据
    static string sha224bin(const string &s);
    static string sha224filebin(const string &sFile);

    // 结果：56字节HEX字符串
    static string sha224hex(const string &s);
    static string sha224filehex(const string &sFile);

    // 结果：32字节二进制数据
    static string sha256bin(const string &s);
    static string sha256filebin(const string &sFile);

    // 结果：64字节HEX字符串
    static string sha256hex(const string &s);
    static string sha256filehex(const string &sFile);

    // 结果：48字节二进制数据
    static string sha384bin(const string &s);
    static string sha384filebin(const string &sFile);

    // 结果：96字节HEX字符串
    static string sha384hex(const string &s);
    static string sha384filehex(const string &sFile);

    // 结果：64字节二进制数据
    static string sha512bin(const string &s);
    static string sha512filebin(const string &sFile);

    // 结果：128字节HEX字符串
    static string sha512hex(const string &s);
    static string sha512filehex(const string &sFile);
};

}
#endif
