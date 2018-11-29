#include "util/util_string.h"
#include "util/util_stl.h"
#include <iostream>
#include <iterator>

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

    string s2 = "GET / HTTP/1.1\r\nOrigin: https://192.168.0.16:0\r\nSec-WebSocket-Key: QQxmh3rcAub8VRrc8IpXpQ==\r\nConnection: Upgrade\r\nUpgrade: websocket\r\nSec-WebSocket-Version: 13\r\nUser-Agent: Mozilla/5.0 (Windows NT 10.0; WOW64; Trident/7.0; rv:11.0) like Gecko\r\nHost: 192.168.0.16:15100\r\nCache-Control: no-cache\r\nCookie: theworld_client_none=\263\261\224\257";
    
    mParam.clear();
    UtilString::splitString2(s2, "\r\n", ": ", mParam);

    cout << UtilString::joinURLParam(mParam, "&", "=") << endl;

    string s3 = "1::2:";
    vector<string> v3 = UtilString::splitString(s3, ":");
    std::copy(v3.begin(), v3.end(), std::ostream_iterator<string>(cout, "|"));
    cout << endl;
    vector<string> v4 = UtilString::splitString(s3, ":", false);
    std::copy(v4.begin(), v4.end(), std::ostream_iterator<string>(cout, "|"));
    cout << endl;

	return 0;
}
