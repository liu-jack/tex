#include <iostream>
#include <sstream>
#include <fstream>
#include <algorithm>
#include "parse/sdptree.h"
#include "util/util_string.h"
#include "util/util_file.h"
#include "util/util_option.h"
using namespace std;
using namespace mfw;
using namespace mfw::sdp;

class Sdp2LuaDoc
{
public:
	Sdp2LuaDoc() : m_bSupportSTLType(false) {}

	void setOutputDir(const string &sDir) { m_sOutputDir = sDir; }
	void setSupportSTLType(bool bSupport) { m_bSupportSTLType = bSupport; }

	void add(const SdpParseTree &stTree)
	{
		for (unsigned i = 0; i < stTree.vNamespace.size(); ++i)
		{
			const SdpNamespace &stNamespace = stTree.vNamespace[i];
			m_mAllNamespace[stNamespace.sNamespaceName].push_back(stNamespace);
		}
	}

	void generate()
	{
		for (map<string, vector<SdpNamespace> >::const_iterator first = m_mAllNamespace.begin(), last = m_mAllNamespace.end(); first != last; ++first)
		{
			const string &sNamespace = first->first;
			const vector<SdpNamespace> &vNamespace = first->second;

			string sSouceFile = "Doc" + sNamespace + ".lua";
			if (!m_sOutputDir.empty())
			{
				UtilFile::makeDirectoryRecursive(m_sOutputDir);
				sSouceFile = m_sOutputDir + "/" + sSouceFile;
			}

			string sSourceContent = generateSource(sNamespace, vNamespace);
			UtilFile::saveToFile(sSouceFile, sSourceContent);
		}
	}

private:
	string generateSource(const string &sNamespace, const vector<SdpNamespace> &vNamespace)
	{
		os.str("");
		os.clear();
		printHeader(sNamespace);
		for (unsigned i = 0; i < vNamespace.size(); ++i)
		{
			const SdpNamespace &stNamespace = vNamespace[i];
			printNamespace(stNamespace);
		}
		return os.str();
	}
	void printHeader(const string &sNamespace)
	{
		os << "module('" << sNamespace << "')" << endl;
		os << "--- @module " << sNamespace << endl << endl;
		os << "--- @field [parent=#global] #" << sNamespace << " " << sNamespace << endl << endl;
	}
	void printNamespace(const SdpNamespace &stNamespace)
	{
		for (unsigned i = 0; i < stNamespace.vEnum.size(); ++i)
		{
			printEnum(stNamespace, stNamespace.vEnum[i]);
		}
		for (unsigned i = 0; i < stNamespace.vStruct.size(); ++i)
		{
			printStruct(stNamespace, stNamespace.vStruct[i]);
		}
	}
	void printEnum(const SdpNamespace &stNamespace, const SdpEnum &stEnum)
	{
		for (unsigned i = 0; i < stEnum.vEnumItem.size(); ++i)
		{
			const SdpEnum::EnumItem &stEnumItem = stEnum.vEnumItem[i];
			os << "--- @field [parent=#" << stNamespace.sNamespaceName << "] #number " << stEnumItem.sName << endl << endl;
		}
	}
	void printStruct(const SdpNamespace &stNamespace, const SdpStruct &stStruct)
	{
		os << "--- @type " << stStruct.sStructName << endl << endl;
		os << "--- @type " << stStruct.sStructName << "_Instance" << endl;
		for (unsigned i = 0; i < stStruct.vStructField.size(); ++i)
		{
			const SdpStruct::StructField &stStructField = stStruct.vStructField[i];
			string sType = getSdpTypeName(stStructField.stFieldType);
			if (sType.empty())
			{
				os << "-- @field " << stStructField.sFieldName << endl;
			}
			else
			{
				os << "-- @field #" << sType << " " << stStructField.sFieldName << endl;
			}
		}
		os << endl;

		os << "--- @function [parent=#" << stStruct.sStructName << "] new" << endl;
		os << "-- @return #" << stStruct.sStructName << "_Instance" << endl << endl;

		os << "--- @function [parent=#" << stStruct.sStructName << "] pack" << endl;
		os << "-- @param o" << endl;
		os << "-- @return #string" << endl << endl;

		os << "--- @function [parent=#" << stStruct.sStructName << "] unpack" << endl;
		os << "-- @param #string data" << endl;
		os << "-- @return #" << stStruct.sStructName << "_Instance" << endl << endl;

		os << "--- @callof #" << stStruct.sStructName << endl;
		os << "-- @return #" << stStruct.sStructName << "_Instance" << endl << endl;

		os << "--- @field [parent=#" << stNamespace.sNamespaceName << "] #" << stStruct.sStructName << " " << stStruct.sStructName << endl << endl;
	}

private:
	string getSdpTypeName(const SdpType &stType)
	{
		switch (stType.iTypeId)
		{
		case SdpType_Void: return "";
		case SdpType_Bool: return "boolean";
		case SdpType_Char:
		case SdpType_Int8:
		case SdpType_UInt8:
		case SdpType_Int16:
		case SdpType_UInt16:
		case SdpType_Int32:
		case SdpType_UInt32:
		case SdpType_Float:
		case SdpType_Double:
		case SdpType_Enum:
			return "number";
		case SdpType_Int64:
		case SdpType_UInt64:
		case SdpType_String:
			return "string";
		case SdpType_Vector:
			if (m_bSupportSTLType)
			{
				return "list<#" + getSdpTypeName(stType.vInnerType[0]) + ">";
			}
			else
			{
				return "table";
			}
		case SdpType_Map:
			if (m_bSupportSTLType)
			{
				return "map<#" + getSdpTypeName(stType.vInnerType[0]) + ", #" + getSdpTypeName(stType.vInnerType[1]) + ">";
			}
			else
			{
				return "table";
			}
		case SdpType_Struct:
			return stType.sName + "_Instance";
		}
		return "";
	}

private:
	map<string, vector<SdpNamespace> > m_mAllNamespace;
	ostringstream 		os;
	string				m_sOutputDir;
	bool				m_bSupportSTLType;
};

int main(int argc, char *argv[])
{
	COption option;
	option.parse(argc, argv);
	if (option.getParam().empty())
	{
		cerr << "Usage: " << argv[0] << " [--dir=] [--nosort] [--stltype] sdpfile" << endl;
		return -1;
	}

	Sdp2LuaDoc sdp2luadoc;
	if (option.hasOption("dir"))
	{
		sdp2luadoc.setOutputDir(option.getOption("dir"));
	}
	if (option.hasOption("stltype"))
	{
		sdp2luadoc.setSupportSTLType(true);
	}

	vector<string> vFiles = option.getParam();
	if (!option.hasOption("nosort"))
	{
		sort(vFiles.begin(), vFiles.end());
	}

	for (unsigned i = 0; i < vFiles.size(); ++i)
	{
		SdpParseTree stTree;
		int ret = parsesdp(vFiles[i], stTree);
		if (ret != 0)
		{
			cerr << "fail to parse file: " << vFiles[i] << endl;
			continue;
		}

		sdp2luadoc.add(stTree);
	}
	sdp2luadoc.generate();
	return 0;
}
