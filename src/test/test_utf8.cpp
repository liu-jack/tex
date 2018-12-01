#include "utf8/utf8.h"
#include <stdint.h>
#include <iostream>
#include <vector>

using namespace std;

int main(int argc, char* argv[])
{

    string s1 = "靐";
    cout << s1.size() << endl;

    string s2 = "\xE9\x9D\x90";
    cout << s2 << s2.size() << endl;

    string::const_iterator it1 = s1.begin();
    string::const_iterator it2 = s1.end();
    uint32_t cp = utf8::next(it1, it2);
    cout << cp << endl;

    vector<uint16_t> utf16;
    it1 = s1.begin();
    utf8::utf8to16(it1, it2, std::back_inserter(utf16));
    cout << utf16.size() << endl;

    string s3;
    utf8::utf16to8(utf16.begin(), utf16.end(), std::back_inserter(s3));
    cout << s3 << endl;

    return 0;
}
