#include "util/util_json.h"
#include <iostream>

using namespace mfw;
using namespace std;

int main(int argc, char* argv[])
{
    if (argc != 2) {
        cout << "Usage:" << argv[0] << " jsonstring" << endl;
        return -1;
    }

    string sJson = argv[1];

    Json::Value json;
    string sError;
    if (!UtilJson::stringToJson(sJson, json, sError)) {
        cout << "invalid json string: " << sJson << ",error: " << sError;
        return -1;
    }

    Json::StyledWriter writer;
    cout << "json string: " << writer.write(json) << endl;
    cout << "json to lua: " << UtilJson::jsonToLua(json) << endl;

    return 0;
}
