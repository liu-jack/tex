#include "util_json.h"
#include "util_string.h"

namespace mfw
{

string UtilJson::jsonToString(const Json::Value &json)
{
	Json::FastWriter writer;
	return writer.write(json);	
}

string UtilJson::jsonToStringStyled(const Json::Value &json)
{
	Json::StyledWriter writer;
	return writer.write(json);
}

void UtilJson::stringToJson(const string &sData, Json::Value &json)
{
	if (sData.empty()) return;
	Json::Reader reader;
	reader.parse(sData, json, false);
}

bool UtilJson::stringToJson(const string &sData, Json::Value &json, string &sErrorInfo)
{
	if (sData.empty()) return true;

	Json::Reader reader;
	if (!reader.parse(sData, json, false))
	{
		sErrorInfo = reader.getFormatedErrorMessages();
		return false;
	}

	return true;
}

string UtilJson::jsonToLua(const Json::Value &json)
{
	using namespace Json;

	switch (json.type())
	{
	case nullValue:	return "nil";
	case intValue: return UtilString::tostr(json.asInt());
	case uintValue: return UtilString::tostr(json.asUInt());
	case realValue: return UtilString::tostr(json.asDouble());
	case stringValue: return "'"+json.asString()+"'";
	case booleanValue: return json.asBool() ? "true" : "false";
	case arrayValue:
		{
			std::ostringstream os;
			os << "{";
			for (uint32_t i = 0; i < json.size(); ++i)
			{
				if (i > 0) os << ",";
				os << "[" << i+1 << "]" << " = ";
				os << jsonToLua(json[i]);
			}
			os << "}";
			return os.str();
		}
	case objectValue:
		{
			std::ostringstream os;	
			Value::Members members(json.getMemberNames());
          	os << "{";
			for ( Value::Members::iterator it = members.begin(); 
	        	it != members.end(); 
                ++it )
          	{
            	const std::string &name = *it;
             	if ( it != members.begin() )
                	os << ",";
             os << "[\"" << name.c_str() << "\"]" << " = ";
             os << jsonToLua(json[name]);
          	}
          	os << "}";
			return os.str();
		}
	}

	return string();
}

void UtilJson::joinJson(Json::Value &jsonTarget, const Json::Value &jsonSource, JoinJsonType type)
{
}

}
