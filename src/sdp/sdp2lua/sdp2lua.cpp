#include <iostream>
#include <sstream>
#include <fstream>
#include "parse/sdptree.h"
#include "util/util_string.h"
#include "util/util_file.h"
#include "util/util_option.h"
using namespace std;
using namespace mfw;
using namespace mfw::sdp;

class Sdp2Lua
{
public:
	explicit Sdp2Lua(const SdpParseTree &stTree) : m_stTree(stTree) {}

	void setOutputDir(const string &sDir) { m_sOutputDir = sDir; }

	void generate()
	{
		string sSouceFile = UtilFile::replaceFileExt(UtilFile::getFileBasename(m_stTree.sFileName), ".lua", true);
		if (!m_sOutputDir.empty())
		{
			UtilFile::makeDirectoryRecursive(m_sOutputDir);
			sSouceFile = m_sOutputDir + "/" + sSouceFile;
		}

		string sSourceContent = generateSource();
		UtilFile::saveToFile(sSouceFile, sSourceContent);
	}

	string generateSource()
	{
		os.str("");
		os.clear();
		printHeader();
		printNamespace();
		return os.str();
	}

private:
	void printHeader()
	{
		os << "local module = module" << endl;
		os << "local _G = _G" << endl;
		os << "local sdp = require 'sdp'" << endl;

		for (unsigned i = 0; i < m_stTree.vInclude.size(); ++i)
		{
			string sModule = UtilFile::replaceFileExt(UtilFile::getFileBasename(m_stTree.vInclude[i]), "", true);
			os << "local " << sModule << " = require('" << sModule << "')" << endl;
		}
	}
	void printNamespace()
	{
		for (unsigned i = 0; i < m_stTree.vNamespace.size(); ++i)
		{
			const SdpNamespace &stNamespace = m_stTree.vNamespace[i];
			os << "module('" << stNamespace.sNamespaceName << "')" << endl << endl;

			for (unsigned i = 0; i < stNamespace.vEnum.size(); ++i)
			{
				printEnum(stNamespace.vEnum[i]);
			}
			for (unsigned i = 0; i < stNamespace.vStruct.size(); ++i)
			{
				printStruct(stNamespace.vStruct[i]);
			}
		}
	}
	void printEnum(const SdpEnum &stEnum)
	{
		for (unsigned i = 0; i < stEnum.vEnumItem.size(); ++i)
		{
			const SdpEnum::EnumItem &stEnumItem = stEnum.vEnumItem[i];
			os << stEnumItem.sName << " = ";
			if (stEnumItem.bHasAssignValue)
			{
				os << getSdpAssignValue(stEnumItem.stAssignValue);
			}
			else
			{
				if (i == 0)
				{
					os << 0 << endl;
				}
				else
				{
					os << stEnum.vEnumItem[i - 1].sName << " + 1";
				}
			}
			os << endl;
		}
		if (!stEnum.vEnumItem.empty())
		{
			os << endl;
		}
	}
	void printStruct(const SdpStruct &stStruct)
	{
		os << stStruct.sStructName << " = sdp.SdpStruct('" << stStruct.sStructName << "')" << endl;
		os << stStruct.sStructName << ".Definition = {" << endl;

		if (!stStruct.vStructField.empty())
		{
			os << "  ";
		}
		for (unsigned i = 0; i < stStruct.vStructField.size(); ++i)
		{
			const SdpStruct::StructField &stStructField = stStruct.vStructField[i];
			os << "'" << stStructField.sFieldName << "', ";
		}
		if (!stStruct.vStructField.empty())
		{
			os << endl;
		}

		for (unsigned i = 0; i < stStruct.vStructField.size(); ++i)
		{
			const SdpStruct::StructField &stStructField = stStruct.vStructField[i];
			os << "  " << stStructField.sFieldName << " = {";
			os << stStructField.iTag << ", " << (stStructField.bRequired ? 1 : 0)
				<< ", " << getSdpTypeName(stStructField.stFieldType)
				<< ", " << getStructFieldDefaultValue(stStructField);
			os << "}," << endl;
		}

		os << "}" << endl << endl;
	}

	string getStructFieldDefaultValue(const SdpStruct::StructField &stStructField)
	{
		string sDefault;
		switch (stStructField.stFieldType.iTypeId)
		{
		case SdpType_Bool:
			{
				if (stStructField.bHasDefaultValue)
				{
					sDefault = getSdpAssignValue(stStructField.stDefaultValue);
				}
				else
				{
					sDefault = "false";
				}
			}
			break;
		case SdpType_Char:
		case SdpType_Int8:
		case SdpType_UInt8:
		case SdpType_Int16:
		case SdpType_UInt16:
		case SdpType_Int32:
		case SdpType_UInt32:
		case SdpType_Float:
		case SdpType_Double:
			{
				if (stStructField.bHasDefaultValue)
				{
					sDefault = getSdpAssignValue(stStructField.stDefaultValue);
				}
				else
				{
					sDefault = "0";
				}
			}
			break;
		case SdpType_Int64:
		case SdpType_UInt64:
			{
				if (stStructField.bHasDefaultValue)
				{
					sDefault = "'" + getSdpAssignValue(stStructField.stDefaultValue) + "'";
				}
				else
				{
					sDefault = "'0'";
				}
			}
			break;
		case SdpType_String:
			{
				if (stStructField.bHasDefaultValue)
				{
					sDefault = getSdpAssignValue(stStructField.stDefaultValue);
				}
				else
				{
					sDefault = "''";
				}
			}
			break;
		case SdpType_Enum:
			{
				if (stStructField.bHasDefaultValue)
				{
					sDefault = getSdpAssignValue(stStructField.stDefaultValue);
				}
				else
				{
					sDefault = "0";
				}
			}
			break;
		case SdpType_Vector:
		case SdpType_Map:
		case SdpType_Struct:
			sDefault = "nil";
			break;
		default:
			break;
		}
		return sDefault;
	}

private:
	static string getSdpTypeName(const SdpType &stType)
	{
		switch (stType.iTypeId)
		{
		case SdpType_Void:
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
		case SdpType_String:
		case SdpType_Enum:
			return UtilString::tostr(stType.iTypeId);
		case SdpType_Vector:
			{
				string s = getSdpTypeName(stType.vInnerType[0]);
				return "sdp.SdpVector(" + s + ")";
			}
		case SdpType_Map:
			{
				string s1 = getSdpTypeName(stType.vInnerType[0]);
				string s2 = getSdpTypeName(stType.vInnerType[1]);
				return "sdp.SdpMap(" + s1 + ", " + s2 + ")";
			}
		case SdpType_Struct:
			{
				if (stType.sName.find("::") == string::npos)
				{
					return stType.sName;
				}
				return "_G." + UtilString::replace(stType.sName, "::", ".");
			}
		}
		return "";
	}

	static string getSdpAssignValue(const SdpAssignValue &stValue)
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

private:
	const SdpParseTree 	&m_stTree;
	ostringstream 		os;
	string				m_sOutputDir;
};

int main(int argc, char *argv[])
{
	COption option;
	option.parse(argc, argv);
	if (option.getParam().empty())
	{
		cerr << "Usage: " << argv[0] << " [--dir=] sdpfile" << endl;
		return -1;
	}

	for (unsigned i = 0; i < option.getParam().size(); ++i)
	{
		SdpParseTree stTree;
		int ret = parsesdp(option.getParam()[i], stTree);
		if (ret != 0)
		{
			cerr << "fail to parse file: " << argv[i] << endl;
			continue;
		}

		//cout << dumpsdp(stTree) << endl;

		Sdp2Lua sdp2lua(stTree);
		if (option.hasOption("dir"))
		{
			sdp2lua.setOutputDir(option.getOption("dir"));
		}
		sdp2lua.generate();
	}
	return 0;
}
