#ifdef IMPLEMENT_INCLUDE

#define CONCAT(x, y) x ## y
#define CONCAT2(x, y) CONCAT(x, y)

#define FUNC_BIN		CONCAT2(DIGEST_NAME, bin)
#define FUNC_FILEBIN	CONCAT2(DIGEST_NAME, filebin)
#define FUNC_HEX		CONCAT2(DIGEST_NAME, hex)
#define FUNC_FILEHEX	CONCAT2(DIGEST_NAME, filehex)
#define FUNC_DIGEST_BUFFER CONCAT2(DIGEST_NAME, _buffer)
#define FUNC_DIGEST_STREAM CONCAT2(DIGEST_NAME, _stream)

string UtilSHA::FUNC_BIN(const string &s)
{
    char result[DIGEST_RESULT_SIZE];
    FUNC_DIGEST_BUFFER(s.c_str(), s.size(), result);
    return string(result, result + sizeof(result));
}

string UtilSHA::FUNC_FILEBIN(const string &sFile)
{
    FILE *fp = fopen(sFile.c_str(), "rb");
    if (fp == NULL) {
        return "";
    }

    char result[DIGEST_RESULT_SIZE];
    int ret = FUNC_DIGEST_STREAM(fp, result);
    fclose(fp);

    if (ret != 0) {
        return "";
    }
    return string(result, result + sizeof(result));
}

string UtilSHA::FUNC_HEX(const string &s)
{
    return UtilEncode::toHex(FUNC_BIN(s));
}

string UtilSHA::FUNC_FILEHEX(const string &sFile)
{
    return UtilEncode::toHex(FUNC_FILEBIN(sFile));
}

#undef DIGEST_NAME
#undef DIGEST_RESULT_SIZE

#else
#include "util_sha.h"
#include "util_encode.h"
#include "util_impl/sha1.h"
#include "util_impl/sha256.h"
#include "util_impl/sha512.h"

namespace mfw
{

#define IMPLEMENT_INCLUDE

#define DIGEST_NAME			sha1
#define DIGEST_RESULT_SIZE	SHA1_DIGEST_SIZE
#include "util_sha.cpp"

#define DIGEST_NAME			sha224
#define DIGEST_RESULT_SIZE	SHA224_DIGEST_SIZE
#include "util_sha.cpp"

#define DIGEST_NAME			sha256
#define DIGEST_RESULT_SIZE	SHA256_DIGEST_SIZE
#include "util_sha.cpp"

#define DIGEST_NAME			sha384
#define DIGEST_RESULT_SIZE	SHA384_DIGEST_SIZE
#include "util_sha.cpp"

#define DIGEST_NAME			sha512
#define DIGEST_RESULT_SIZE	SHA512_DIGEST_SIZE
#include "util_sha.cpp"

}

#endif
