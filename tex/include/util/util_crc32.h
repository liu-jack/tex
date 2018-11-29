#ifndef _MFW_UTIL_CRC32_
#define _MFW_UTIL_CRC32_

#include <stdint.h>
#include <string>
using namespace std;

namespace mfw
{

class UtilCRC32
{
public:
    static uint32_t crc32(const string &s);
    static uint32_t crc32file(const string &sFile);

    // 结果：4字节二进制数据
    static string crc32bin(const string &s);
    static string crc32filebin(const string &sFile);

    // 结果：8字节HEX字符串
    static string crc32hex(const string &s);
    static string crc32filehex(const string &sFile);
};

}
#endif
