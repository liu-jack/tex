#ifndef _MFW_WEBSOCKET_H_
#define _MFW_WEBSOCKET_H_

#include <string>

using namespace std;

namespace mfw
{

class Websocket
{
public:
    static int handshake(const char *pInBegin, const char *pInEnd, const char *&pNextIn, string &sOut);
    static int parseProtocol(const char *pInBegin, const char *pInEnd, const char *&pNextIn, string &sOut);
    static string encodeProtocol(const string &sBuffer);
};

};

#endif
