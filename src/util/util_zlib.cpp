#include "util_zlib.h"
#include "zlib/zlib.h"
#include <cstring>
#include <stdexcept>

#define CHUNK_SIZE (32 * 1024)

namespace mfw
{

string UtilZlib::zlib_compress(const string &sData, int level)
{
    string sResult;
    if (!zlib_compress(sData, sResult, level)) {
        throw std::runtime_error("zlib compress");
    }
    return sResult;
}

string UtilZlib::zlib_uncompress(const string &sData)
{
    string sResult;
    if (!zlib_uncompress(sData, sResult)) {
        throw std::runtime_error("zlib uncompress");
    }
    return sResult;
}
bool UtilZlib::zlib_compress(const string &sData, string &sResult, int level)
{
    return zlib_compress(sData.c_str(), sData.c_str() + sData.size(), sResult, level);
}

bool UtilZlib::zlib_uncompress(const string &sData, string &sResult)
{
    return zlib_uncompress(sData.c_str(), sData.c_str() + sData.size(), sResult);
}

bool UtilZlib::zlib_compress(const char *pDataBegin, const char *pDataEnd, string &sResult, int level)
{
    z_stream strm;
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    int ret = deflateInit(&strm, level);
    if (ret != Z_OK) {
        return false;
    }

    unsigned char out[CHUNK_SIZE];
    strm.avail_in = pDataEnd - pDataBegin;
    strm.next_in = (unsigned char *)pDataBegin;
    do {
        strm.avail_out = sizeof(out);
        strm.next_out = out;
        deflate(&strm, Z_FINISH);
        sResult.append((char *)out, sizeof(out) - strm.avail_out);
    } while (strm.avail_out == 0);
    deflateEnd(&strm);
    return true;
}

bool UtilZlib::zlib_uncompress(const char *pDataBegin, const char *pDataEnd, string &sResult)
{
    z_stream strm;
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = 0;
    strm.next_in = Z_NULL;
    int ret = inflateInit(&strm);
    if (ret != Z_OK) {
        return false;
    }

    unsigned char out[CHUNK_SIZE];
    strm.avail_in = pDataEnd - pDataBegin;
    strm.next_in = (unsigned char *)pDataBegin;
    do {
        strm.avail_out = sizeof(out);
        strm.next_out = out;
        ret = inflate(&strm, Z_NO_FLUSH);
        if (ret == Z_STREAM_ERROR || ret == Z_NEED_DICT || ret == Z_DATA_ERROR || ret == Z_MEM_ERROR) {
            inflateEnd(&strm);
            return false;
        }

        sResult.append((char *)out, sizeof(out) - strm.avail_out);
    } while (strm.avail_out == 0);

    inflateEnd(&strm);
    if (ret != Z_STREAM_END) {
        return false;
    }
    return true;
}

bool UtilZlib::isCompressWithPad(const string &sData)
{
    const static unsigned padlen = 4;
    if (sData.size() <= padlen || strncmp(sData.c_str(), "ZLIB", padlen) != 0) {
        return false;
    }
    return true;
}

string UtilZlib::zlib_compressWithPad(const string &sData, int level)
{
    string sResult = "ZLIB";
    if (!zlib_compress(sData, sResult, level)) {
        throw std::runtime_error("zlib compress with pad");
    }
    return sResult;
}

string UtilZlib::zlib_uncompressWithPad(const string &sData)
{
    if (!isCompressWithPad(sData)) {
        throw std::runtime_error("invalid zlib pad");
    }

    const static unsigned padlen = 4;
    string sResult;
    if (!zlib_uncompress(sData.c_str() + padlen, sData.c_str() + sData.size(), sResult)) {
        throw std::runtime_error("zlib uncompress with pad");
    }
    return sResult;
}

}
