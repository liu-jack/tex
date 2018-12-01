#include "util/util_string.h"
#include "util/util_stl.h"
#include <iostream>
#include <iterator>
#include <assert.h>

using namespace mfw;

int main(int argc, char* argv[])
{
    string s = UtilString::format("afdsafsdafdsfdsafdsafdsfsdafsdfsd%s,%u", "test", 111);
    cout << s << endl;

    map<string,string> mParam;
    string s1 = "code=&access_token=0.3087069e8ec1faf72b368b10627b95fb.a36256aea0381364982aa563892c0f96.1484287581857&openid=79803584&expires_in=7776000&refresh_token=0.6629bee39637e921656a8de0a7f010c7";
    UtilString::splitURLParam(s1, mParam);

    const string *ps = UtilSTL::findMapPtr(mParam, "code");
    if (ps)
        cout << "code" << *ps;

    const string *ps2 = UtilSTL::findMapPtr(mParam, "access_token");
    if (ps2)
        cout << "access_token" << *ps2;

    cout << endl;

    string s2 = "GET / HTTP/1.1\r\nOrigin: https://192.168.0.16:0\r\nSec-WebSocket-Key: QQxmh3rcAub8VRrc8IpXpQ==\r\nConnection: Upgrade\r\nUpgrade: websocket\r\nSec-WebSocket-Version: 13\r\nUser-Agent: Mozilla/5.0 (Windows NT 10.0; WOW64; Trident/7.0; rv:11.0) like Gecko\r\nHost: 192.168.0.16:15100\r\nCache-Control: no-cache\r\nCookie: theworld_client_none=";

    mParam.clear();
    vector<string> vParam =  UtilString::splitString(s2, "\r\n");
    for (uint32_t i = 0; i < vParam.size(); ++i) {
        string::size_type p = vParam[i].find_first_of(":");
        if (p == string::npos) {
            cout << vParam[i] << endl;
        } else {
            string sHeadKey = vParam[i].substr(0, p);
            string sHeadValue = vParam[i].substr(p+2);
            cout << sHeadKey << "=" << sHeadValue << endl;
        }
    }

    string s3 = "1::2:";
    vector<string> v3 = UtilString::splitString(s3, ":");
    assert (v3.size() == 2);
    cout << UtilString::joinString(v3, "|") << endl;
    vector<string> v4 = UtilString::splitString(s3, ":", false);
    cout << UtilString::joinString(v4, "|") << endl;

    return 0;
}
