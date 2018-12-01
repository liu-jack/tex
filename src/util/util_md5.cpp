#include "util_md5.h"
#include "util_encode.h"
#include "util_impl/md5.h"

namespace mfw
{

string UtilMD5::md5bin(const string &s)
{
    char result[MD5_DIGEST_SIZE];
    md5_buffer(s.c_str(), s.size(), result);
    return string(result, result + sizeof(result));
}

string UtilMD5::md5filebin(const string &sFile)
{
    FILE *fp = fopen(sFile.c_str(), "rb");
    if (fp == NULL) {
        return "";
    }

    char result[MD5_DIGEST_SIZE];
    int ret = md5_stream(fp, result);
    fclose(fp);

    if (ret != 0) {
        return "";
    }
    return string(result, result + sizeof(result));
}

string UtilMD5::md5hex(const string &s)
{
    return UtilEncode::toHex(md5bin(s));
}

string UtilMD5::md5filehex(const string &sFile)
{
    return UtilEncode::toHex(md5filebin(sFile));
}

}
