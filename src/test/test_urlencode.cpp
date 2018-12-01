#include "util/util_string.h"
#include "util/util_encode.h"
#include <assert.h>

#include <iostream>

using namespace std;
using namespace mfw;

struct TestStr {
    string s;
    string se;
};

int main(int argc, char* argv[])
{
    TestStr ss[] = {
        {"abvafd1134ABD-_.~", "abvafd1134ABD-_.%7e"},
        {" ", "+"},
        {"我是歌手", "%e6%88%91%e6%98%af%e6%ad%8c%e6%89%8b"},
        {"HdVphwzIN9a/R336u0edfEvWrj6dSOaoTZtXbWY3+9Avjq6YENowdngvaN0BTt/Ftl7qjEGQo0qo60wfrcWFztM1s2J2s3pwnP6KbkivOJhWVZjPvYrRmQwhG4mNgbHsfKwBTXlSXkT+9OJKZjCeOt+lgcr1Om6HWYX+kS14Ua4=", "HdVphwzIN9a%2fR336u0edfEvWrj6dSOaoTZtXbWY3%2b9Avjq6YENowdngvaN0BTt%2fFtl7qjEGQo0qo60wfrcWFztM1s2J2s3pwnP6KbkivOJhWVZjPvYrRmQwhG4mNgbHsfKwBTXlSXkT%2b9OJKZjCeOt%2blgcr1Om6HWYX%2bkS14Ua4%3d"},
        {"{\"transtype\":0,\"cporderid\":\"1\",\"transid\":\"2\",\"appuserid\":\"10001\",\"appid\":\"3012542289\",\"waresid\":31,\"feetype\":4,\"money\":5.00,\"currency\":\"RMB\",\"result\":0,\"transtime\":\"2012-12-12 12:11:10\",\"cpprivate\":\"9999_10001_1_1\",\"paytype\":1}", "%7b%22transtype%22%3a0%2c%22cporderid%22%3a%221%22%2c%22transid%22%3a%222%22%2c%22appuserid%22%3a%2210001%22%2c%22appid%22%3a%223012542289%22%2c%22waresid%22%3a31%2c%22feetype%22%3a4%2c%22money%22%3a5.00%2c%22currency%22%3a%22RMB%22%2c%22result%22%3a0%2c%22transtime%22%3a%222012-12-12+12%3a11%3a10%22%2c%22cpprivate%22%3a%229999_10001_1_1%22%2c%22paytype%22%3a1%7d"}
    };

    for (uint32_t i = 0; i < sizeof(ss)/sizeof(ss[0]); ++i) {
        const TestStr &s = ss[i];
        string se = UtilEncode::urlencode(s.s);
        if (se != s.se) {
            cout << s.s << " encode dismatch: " << s.se << "<=>" << se << endl;
        }
        string sd = UtilEncode::urldecode(se);
        if (sd != s.s) {
            cout << s.s << " decode dismatch: " << s.s << "<=>" << sd << endl;
        }
    }

    return 0;
}
