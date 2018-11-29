#ifndef _MFW_UTIL_JSON_H_
#define _MFW_UTIL_JSON_H_

#include <string>
#include "json/json.h"
using namespace std;

namespace mfw
{

class UtilJson
{
public:
	static string jsonToString(const Json::Value &json);
	static string jsonToStringStyled(const Json::Value &json);
	static void stringToJson(const string &sData, Json::Value &json);
	static bool stringToJson(const string &sData, Json::Value &json, string &sErrorInfo);

	static string jsonToLua(const Json::Value &json);

	enum JoinJsonType
	{
		JoinJsonType_OverrideExist_AddNew,
		JoinJsonType_OverrideExist_IgnoreNew,
		JoinJsonType_IgnoreExist_AddNew,
	};

	static void joinJson(Json::Value &jsonTarget, const Json::Value &jsonSource, JoinJsonType type);
};

}

#endif
