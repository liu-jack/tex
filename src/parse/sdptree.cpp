#include "parse.h"
#include "lex.yy.hpp"
#include <sstream>
using namespace std;

namespace mfw
{
namespace sdp
{

int parsesdp(const string &sFileName, SdpParseTree &stTree)
{
	g_parser.cleanup();
	int ret = 0;
	ret = g_parser.pushFile(sFileName);
	if (ret != 0)
	{
		return ret;
	}

	ret = yyparse();
	if (ret != 0)
	{
		return ret;
	}

	ParseContextPtr context = g_parser.getCurContext();
	swap(stTree, context->stTree);
	return 0;
}

string getSdpTypeName(const SdpType &stType)
{
	switch (stType.iTypeId)
	{
	case SdpType_Void: return "void";
	case SdpType_Bool: return "bool";
	case SdpType_Char: return "char";
	case SdpType_Int8: return "int8_t";
	case SdpType_UInt8: return "uint8_t";
	case SdpType_Int16: return "int16_t";
	case SdpType_UInt16: return "uint16_t";
	case SdpType_Int32: return "int32_t";
	case SdpType_UInt32: return "uint32_t";
	case SdpType_Int64: return "int64_t";
	case SdpType_UInt64: return "uint64_t";
	case SdpType_Float: return "float";
	case SdpType_Double: return "double";
	case SdpType_String: return "string";
	case SdpType_Vector: {
		string s = getSdpTypeName(stType.vInnerType[0]);
		if (s[s.size() - 1] == '>') s += " ";
		return "vector<" + s + ">";
	}
	case SdpType_Map: {
		string s1 = getSdpTypeName(stType.vInnerType[0]);
		string s2 = getSdpTypeName(stType.vInnerType[1]);
		if (s2[s2.size() - 1] == '>') s2 += " ";
		return "map<" + s1 + ", " + s2 + ">";
	}
	case SdpType_Enum:
	case SdpType_Struct: return stType.sName;
	}
	return "";
}

string getSdpAssignValue(const SdpAssignValue &stValue)
{
	switch (stValue.iType)
	{
	case SdpAssignValue::ValueType_Boolean: return stValue.bBoolean ? "true" : "false";
	case SdpAssignValue::ValueType_Integer: return stValue.sValue;
	case SdpAssignValue::ValueType_Float: return stValue.sValue;
	case SdpAssignValue::ValueType_String: return stValue.sValue.empty() ? "" : "\"" + stValue.sValue + "\"";
	}
	return "";
}

string dumpsdp(const SdpParseTree &stTree)
{
	ostringstream os;
	os << "FileName: " << stTree.sFileName << endl;
	for (unsigned i = 0; i < stTree.vInclude.size(); ++i)
	{
		os << "Include: " << stTree.vInclude[i] << endl;
	}
	for (unsigned i = 0; i < stTree.vNamespace.size(); ++i)
	{
		const SdpNamespace &stNamespace = stTree.vNamespace[i];
		os << "---------------" << endl;
		os << "Namespace: " << stNamespace.sNamespaceName << endl;

		for (unsigned i = 0; i < stNamespace.vEnum.size(); ++i)
		{
			const SdpEnum &stEnum = stNamespace.vEnum[i];
			os << "Enum: " << stEnum.sEnumName << endl;
			for (unsigned i = 0; i < stEnum.vEnumItem.size(); ++i)
			{
				const SdpEnum::EnumItem &stEnumItem = stEnum.vEnumItem[i];
				os << "  " << stEnumItem.sName;
				if (stEnumItem.bHasAssignValue)
				{
					string sDefault = getSdpAssignValue(stEnumItem.stAssignValue);
					if (!sDefault.empty())
					{
						os << " = " << sDefault;
					}
				}
				os << "," << endl;
			}
		}

		for (unsigned i = 0; i < stNamespace.vStruct.size(); ++i)
		{
			const SdpStruct &stStruct = stNamespace.vStruct[i];
			os << "Struct: " << stStruct.sStructName << endl;
			for (unsigned i = 0; i < stStruct.vStructField.size(); ++i)
			{
				const SdpStruct::StructField &stStructField = stStruct.vStructField[i];
				os << "  " << stStructField.iTag
					         << " " << (stStructField.bRequired ? "required" : "optional")
					         << " " << getSdpTypeName(stStructField.stFieldType)
					         << " " << stStructField.sFieldName;
				if (stStructField.bHasDefaultValue)
				{
					string sDefault = getSdpAssignValue(stStructField.stDefaultValue);
					if (!sDefault.empty())
					{
						os << " = " << sDefault;
					}
				}
				os << ";" << endl;
			}
			if (!stStruct.vSortKey.empty())
			{
				os << "  KEY: " << stStruct.vSortKey[0];
				for (unsigned i = 1; i < stStruct.vSortKey.size(); ++i)
				{
					os << ", " << stStruct.vSortKey[i];
				}
				os << endl;
			}
		}

		for (unsigned i = 0; i < stNamespace.vInterface.size(); ++i)
		{
			const SdpInterface &stInterface = stNamespace.vInterface[i];
			os << "Interface: " << stInterface.sInterfaceName << endl;
			for (unsigned i = 0; i < stInterface.vOperation.size(); ++i)
			{
				const SdpInterface::Operation &stOperation = stInterface.vOperation[i];
				os << "  " << getSdpTypeName(stOperation.stRetType) << " " << stOperation.sOperationName << "(";
				for (unsigned i = 0; i < stOperation.vParam.size(); ++i)
				{
					const SdpInterface::Paramater &stParamater = stOperation.vParam[i];
					if (i != 0)
					{
						os << ", ";
					}

					if (stParamater.bIsOut)
					{
						os << getSdpTypeName(stParamater.stParamType) << " &" << stParamater.sParamName;
					}
					else
					{
						switch (stParamater.stParamType.iTypeId)
						{
						case SdpType_Bool:
						case SdpType_Char:
						case SdpType_Int8:
						case SdpType_UInt8:
						case SdpType_Int16:
						case SdpType_UInt16:
						case SdpType_Int32:
						case SdpType_UInt32:
						case SdpType_Int64:
						case SdpType_UInt64:
						case SdpType_Float:
						case SdpType_Double:
						case SdpType_Enum:
							os << getSdpTypeName(stParamater.stParamType) << " " << stParamater.sParamName;
							break;
						default:
							os << "const " << getSdpTypeName(stParamater.stParamType) << " &" << stParamater.sParamName;
							break;
						}
					}
				}
				os << ");" << endl;
			}
		}

		os << endl;
	}
	return os.str();
}

}
}
