#ifndef _MFW_UTIL_ZLIB_
#define _MFW_UTIL_ZLIB_

#include <string>
using namespace std;

namespace mfw
{

class UtilZlib
{
public:
    static string zlib_compress(const string &sData, int level = -1);
    static string zlib_uncompress(const string &sData);

    static bool zlib_compress(const string &sData, string &sResult, int level = -1);
    static bool zlib_uncompress(const string &sData, string &sResult);

    static bool zlib_compress(const char *pDataBegin, const char *pDataEnd, string &sResult, int level = -1);
    static bool zlib_uncompress(const char *pDataBegin, const char *pDataEnd, string &sResult);

    static bool isCompressWithPad(const string &sData);
    static string zlib_compressWithPad(const string &sData, int level = -1);
    static string zlib_uncompressWithPad(const string &sData);
};

}

#endif
