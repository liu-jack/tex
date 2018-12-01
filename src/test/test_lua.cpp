#include "luamod/LuaModule.h"
#include "util/util_log.h"
#include <iostream>

using namespace mfw;
using namespace std;

int main(int argc, char* argv[]) {

    CLogManager::getInstance()->initLog("./", "-");        

    try {
        LuaValue value;

        value.set(1, 5);
        value.set(3, "test");
        value.set("a", 77543);

        LuaValue::LuaValueProxy p1 = value.add(2);
        p1.set("t", 9);
        p1.set("mod", 1);

        LuaValue::LuaValueProxy p2 = p1.add(4);
        p2.set("thou", 9);
        p2.set("w", 10000);
        p2.set("143", "gil");

        p1.set("what", "why");

        value.set("apr", 4);

        cout << value.toString() << endl;

        LuaValue v1;
        v1.set(1);
        cout << v1.toString() << endl;

        LuaValue v2;
        v2.set(1.1);
        cout << v2.toString() << endl;

        LuaValue v3;
        v3.set("hello world");
        cout << v3.toString() << endl;

        LuaValue v4;
        cout << v4.toString() << endl;

        LuaModule luamod;
        luamod.addPath("./");
        luamod.doFile("./init.lua");

        LuaValue vv;
        luamod.doCallRet("test_luavalue", value, tr1::tie(vv));
        cout << vv.toString() << endl;
    } catch (std::exception &e) {
        cout << e.what() << endl;
    }

    CLogManager::getInstance()->finiLog();

    return 0;
}
