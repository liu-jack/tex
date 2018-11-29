#include "sdp/Sdp.h"
#include "Test.h"
#include <iostream>
#include <sstream>
#include <assert.h>
#include <stdint.h>

using namespace mfw;
using namespace std;

void printHex(const string &sData) {
    ostringstream os;
    for (uint32_t i = 0; i < sData.size(); ++i) {
        int32_t u = (uint8_t)sData[i];
        if (u <= 0xF)  os << "0";
        os << hex << u;
        if ((i+1)%16==0) os << " ";
    }
    cout << UtilString::toupper(os.str()) << endl;
}

int main(int argc, char* argv[]) {

    try {
        SdpPacker packer;

        packer.pack(0, true, false);
        packer.pack(1, true, false);
        packer.pack(19, true, true);

        packer.pack(20, true, (int8_t)-128);
        packer.pack(21, true, (int8_t)0);
        packer.pack(22, true, (int8_t)127);
        packer.pack(23, true, (uint8_t)0);
        packer.pack(24, true, (uint8_t)255);

        packer.pack(25, true, (int16_t)-32768);
        packer.pack(26, true, (int16_t)32767);
        packer.pack(27, true, (uint16_t)0);
        packer.pack(28, true, (uint16_t)65535);

        packer.pack(29, true, (int32_t)-2147483648);
        packer.pack(30, true, 2147483647);
        packer.pack(31, true, (uint32_t)0);
        packer.pack(32, true, (uint32_t)4294967295);

        packer.pack(33, true, (int64_t)0x7fffffffffffffff);
        packer.pack(34, true, (int64_t)-0x8000000000000000);
        packer.pack(35, true, (uint64_t)0xffffffffffffffff);

        packer.pack(36, true, (float)-1.02);
        packer.pack(37, true, (float)110.4);

        packer.pack(38, true, (double)10000.4);
        packer.pack(39, true, (double)-9802883.01);

        packer.pack(40, true, string("我是字符串"));
        packer.pack(41, true, string("adb"));

        vector<bool> vb;
        vb.push_back(true);vb.push_back(false);
        packer.pack(42, true, vb);

        vector<int8_t> vi8;
        vi8.push_back(127);vi8.push_back(-128);
        packer.pack(43, true, vi8);
        vector<uint8_t> vui8;
        vui8.push_back(0);vui8.push_back(255);
        packer.pack(44, true, vui8);

        vector<int16_t> vi16;
        vi16.push_back(32767);vi16.push_back(-32768);
        packer.pack(45, true, vi16);
        vector<uint16_t> vui16;
        vui16.push_back(0);vui16.push_back(65535);
        packer.pack(46, true, vui16);

        vector<int32_t> vi32;
        vi32.push_back(2147483647);vi32.push_back(-2147483648);
        packer.pack(47, true, vi32);
        vector<uint32_t> vui32;
        vui32.push_back(0);vui32.push_back(4294967295);
        packer.pack(48, true, vui32);

        vector<int64_t> vi64;
        vi64.push_back(-0x8000000000000000);vi64.push_back(0x7fffffffffffffff);
        packer.pack(49, true, vi64);
        vector<uint64_t> vui64;
        vui64.push_back(0);vui64.push_back(0xffffffffffffffff);
        packer.pack(50, true, vui64);

        vector<float> vf1;
        vf1.push_back(1.2);vf1.push_back(-4.5);vf1.push_back(14.5);
        packer.pack(51, true, vf1);

        vector<double> vd1;
        vd1.push_back(1.23);vd1.push_back(-4.51);vd1.push_back(140.5);
        packer.pack(52, true, vd1);

        vector<string> vs;
        vs.push_back("yellia");vs.push_back("hello");vs.push_back("world");
        packer.pack(53, true, vs);

        map<string,string> m;
        m["k1"] = "v1";
        m["k2"] = "v2";
        packer.pack(54, true, m);

        map<uint32_t,string> m2;
        m2[111] = "v1";
        m2[111] = "v2";
        m2[22] = "v2";
        packer.pack(55, true, m2);

        map<uint32_t,uint32_t> m3;
        m3[111] = 123;
        m3[22] = 91;
        packer.pack(56, true, m3);

        map<int32_t,uint32_t> m4;
        m4[-111] = 123;
        m4[22] = 91;
        packer.pack(57, true, m4);

        map<int32_t,uint64_t> m5;
        m5[22] = 0x1234;
        m5[12] = 0x2345;
        packer.pack(58, true, m5);

        // 测试结构体
        Test::Student s1;
        s1.iUid = 1234567890;
        s1.sName = "学生1";
        s1.iAge = 12;
        s1.mSecret["arank"] = "1";
        s1.mSecret["bhasphone"] = "false";
        packer.pack(59, true, s1);

        Test::Student s2;
        s2.iUid = 1234;
        s2.sName = "学生2";
        s2.iAge = 10;
        s2.mSecret["rank"] = "2";

        Test::Teacher t1;
        t1.iId = 1;
        t1.sName = "老师1";
        Test::Teacher t2;
        t2.iId = 2;
        t2.sName = "老师2";
        Test::Teachers ts;
        ts.vTeacher.push_back(t1);
        ts.vTeacher.push_back(t2);

        Test::Class c1;
        c1.iId = 1;
        c1.sName = "一年级一班";
        c1.vStudent.push_back(s1);
        c1.vStudent.push_back(s2);
        string sData = sdpToString(ts);
        std::copy(sData.begin(), sData.end(), std::back_inserter(c1.vData));
        packer.pack(60, true, c1);

        printHex(packer.getData());

        SdpUnpacker unpacker(packer.getData());

        bool b;
        unpacker.unpack(0, true, NULL, b);assert(b == false);
        unpacker.unpack(1, true, NULL, b);assert(b == false);
        unpacker.unpack(19, true, NULL, b);assert(b == true);

        int8_t i8;
        unpacker.unpack(20, true, NULL, i8);assert(i8 == -128);
        unpacker.unpack(21, true, NULL, i8);assert(i8 == 0);
        unpacker.unpack(22, true, NULL, i8);assert(i8 == 127);
        uint8_t ui8;
        unpacker.unpack(23, true, NULL, ui8);assert(ui8 == 0);
        unpacker.unpack(24, true, NULL, ui8);assert(ui8 == 255);

        int16_t i16;
        unpacker.unpack(25, true, NULL, i16);assert(i16 == -32768);
        unpacker.unpack(26, true, NULL, i16);assert(i16 == 32767);
        uint16_t ui16;
        unpacker.unpack(27, true, NULL, ui16);assert(ui16 == 0);
        unpacker.unpack(28, true, NULL, ui16);assert(ui16 == 65535);

        int32_t i32;
        unpacker.unpack(29, true, NULL, i32);assert(i32 == -2147483648);
        unpacker.unpack(30, true, NULL, i32);assert(i32 == 2147483647);
        uint32_t ui32;
        unpacker.unpack(31, true, NULL, ui32);assert(ui32 == 0);
        unpacker.unpack(32, true, NULL, ui32);assert(ui32 == 4294967295);

        int64_t i64;
        unpacker.unpack(33, true, NULL, i64);assert(i64 == 0x7fffffffffffffff);
        unpacker.unpack(34, true, NULL, i64);assert(i64 == -0x8000000000000000);
        uint64_t ui64;
        unpacker.unpack(35, true, NULL, ui64);assert(ui64 == 0xffffffffffffffff);

        float f1;
        unpacker.unpack(36, true, NULL, f1);cout << f1 << endl;
        unpacker.unpack(37, true, NULL, f1);cout << f1 << endl;

        double d1;
        unpacker.unpack(38, true, NULL, d1);cout << d1 << endl;
        unpacker.unpack(39, true, NULL, d1);cout << d1 << endl;

        string us1;
        unpacker.unpack(40, true, NULL, us1);cout << us1 << endl;
        unpacker.unpack(41, true, NULL, us1);cout << us1 << endl;

        Test::Student ut1;
        unpacker.unpack(59, true, NULL, ut1);cout << printSdp(ut1) << endl;

        Test::Class uc1;
        unpacker.unpack(60, true, NULL, uc1);cout << printSdp(uc1) << endl;

        Test::Teachers utt2;
        stringToSdp(string(uc1.vData.begin(), uc1.vData.end()), utt2);
        cout << printSdp(utt2) << endl;
    } catch (exception &e) {
        cout << e.what() << endl;
    }

    return 0;
}
