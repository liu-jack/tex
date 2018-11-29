#ifndef _MFW_UTIL_TEA_
#define _MFW_UTIL_TEA_

#include <stdint.h>
#include <string>
using namespace std;

namespace mfw
{

class UtilTEA
{
public:
    static string tea_encrypt(const string &sData, const string &sKey);
    static string tea_decrypt(const string &sData, const string &sKey);
    static bool tea_encrypt(const string &sData, const string &sKey, string &sResult);
    static bool tea_decrypt(const string &sData, const string &sKey, string &sResult);

    static string xtea_encrypt(const string &sData, const string &sKey);
    static string xtea_decrypt(const string &sData, const string &sKey);
    static bool xtea_encrypt(const string &sData, const string &sKey, string &sResult);
    static bool xtea_decrypt(const string &sData, const string &sKey, string &sResult);
};

}

#endif
