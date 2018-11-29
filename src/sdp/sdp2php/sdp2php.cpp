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

class Sdp2Php
{
public:
	explicit Sdp2Php(const SdpParseTree &stTree) : m_stTree(stTree), m_bFlatIncludeFilePath(false) {}

	void setOutputDir(const string &sDir) { m_sOutputDir = sDir; }
	void setFlatIncludeFilePath(bool bFlag) { m_bFlatIncludeFilePath = bFlag; }

	void generate()
	{
		string sSouceFile = UtilFile::replaceFileExt(UtilFile::getFileBasename(m_stTree.sFileName), ".php", true);
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
		os << "<?php" << endl;
	}
	void printNamespace()
	{
		for (unsigned i = 0; i < m_stTree.vNamespace.size(); ++i)
		{
			const SdpNamespace &stNamespace = m_stTree.vNamespace[i];
			os << "namespace " << stNamespace.sNamespaceName << endl;
			os << "{" << endl;
			if (i == 0)
			{
				bool bHasInclude = false;
				for (unsigned i = 0; i < m_stTree.vInclude.size(); ++i)
				{
					string sModule = UtilFile::replaceFileExt(m_stTree.vInclude[i], ".php", true);
					if (m_bFlatIncludeFilePath)
					{
						string::size_type pos = sModule.rfind('/');
						if (pos != string::npos)
						{
							sModule = sModule.substr(pos + 1);
						}
					}
					os << "require_once '" << sModule << "';" << endl;
					bHasInclude = true;
				}
				if (m_stTree.hasInterface())
				{
					os << "require_once 'mfw/MfwClient.php';" << endl;
					bHasInclude = true;
				}
				if (bHasInclude)
				{
					os << endl;
				}
			}

			for (unsigned i = 0; i < stNamespace.vEnum.size(); ++i)
			{
				printEnum(stNamespace, stNamespace.vEnum[i]);
			}
			for (unsigned i = 0; i < stNamespace.vStruct.size(); ++i)
			{
				printStruct(stNamespace.vStruct[i]);
			}
			for (unsigned i = 0; i < stNamespace.vInterface.size(); ++i)
			{
				printInterface(stNamespace.vInterface[i]);
			}
			os << "}" << endl;
		}
	}
	void printEnum(const SdpNamespace &stNamespace, const SdpEnum &stEnum)
	{
		for (unsigned i = 0; i < stEnum.vEnumItem.size(); ++i)
		{
			const SdpEnum::EnumItem &stEnumItem = stEnum.vEnumItem[i];
			os << "define('" << stNamespace.sNamespaceName << "_" << stEnumItem.sName << "', ";
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
					os << stNamespace.sNamespaceName << "_" << stEnum.vEnumItem[i - 1].sName << " + 1";
				}
			}
			os << ");" << endl;
		}
		if (!stEnum.vEnumItem.empty())
		{
			os << endl;
		}
	}
	void printStruct(const SdpStruct &stStruct)
	{
		os << "class "<< stStruct.sStructName << endl;
		os << "{" << endl;
		for (unsigned i = 0; i < stStruct.vStructField.size(); ++i)
		{
			const SdpStruct::StructField &stStructField = stStruct.vStructField[i];
			tab(1) << "public $" << stStructField.sFieldName << ";" << endl;
		}
		os << endl;

		tab(1) << "function __construct()" << endl;
		tab(1) << "{" << endl;
		for (unsigned i = 0; i < stStruct.vStructField.size(); ++i)
		{
			const SdpStruct::StructField &stStructField = stStruct.vStructField[i];
			tab(2) << "$this->" << stStructField.sFieldName << " = " << getStructFieldDefaultValue(stStructField, true) << ";" << endl;
		}
		tab(1) << "}" << endl;

		tab(1) << "public static $__Definition = array(" << endl;
		if (!stStruct.vStructField.empty())
		{
			tab(2);
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
			tab(2) << "'" << stStructField.sFieldName << "' => array("
					<< stStructField.iTag << ", " << (stStructField.bRequired ? 1 : 0)
					<< ", '" << getSdpTypeName(stStructField.stFieldType) << "'"
					<< ", " << getStructFieldDefaultValue(stStructField, false)
					<< ")," << endl;
		}
		tab(1) << ");" << endl;

		os << "}" << endl << endl;
	}
	void printInterface(const SdpInterface &stInterface)
	{
		tab(0) << "class " << stInterface.sInterfaceName << " extends \\mfw\\MfwClient" << endl;
		tab(0) << "{" << endl;
		for (unsigned i = 0; i < stInterface.vOperation.size(); ++i)
		{
			const SdpInterface::Operation &stOperation = stInterface.vOperation[i];
			if (i != 0)
			{
				os << endl;
			}

			bool bHasOutParam = false;
			tab(1) << "public function " << stOperation.sOperationName<< "(";
			for (unsigned i = 0; i < stOperation.vParam.size(); ++i)
			{
				const SdpInterface::Paramater &stParam = stOperation.vParam[i];
				if (i != 0)
				{
					os << ", ";
				}
				if (stParam.bIsOut)
				{
					os << "&";
					bHasOutParam = true;
				}
				os << "$" << stParam.sParamName;
			}
			os << ")" << endl;
			tab(1) << "{" << endl;

			tab(2) << "$_sReqPayload_ = '';" << endl;
			bool bNewLine = false;
			for (unsigned i = 0; i < stOperation.vParam.size(); ++i)
			{
				const SdpInterface::Paramater &stParam = stOperation.vParam[i];
				if (!stParam.bIsOut)
				{
					tab(2) << "$_sReqPayload_ .= sdpToString($" << stParam.sParamName
							<< ", '" << getSdpTypeName(stParam.stParamType) << "', " << (i + 1) << ");" << endl;
					bNewLine = true;
				}
			}
			if (bNewLine)
			{
				os << endl;
			}

			tab(2) << "$_stRspPacket_ = null;" << endl;
			tab(2) << "$this->invoke('" << stOperation.sOperationName << "', $_sReqPayload_, $_stRspPacket_);" << endl;

			bool bHasReturn = stOperation.stRetType.iTypeId != SdpType_Void;
			if (bHasReturn || bHasOutParam)
			{
				os << endl;

				string sListStr;
				vector<string> vTypeStr;
				if (bHasReturn)
				{
					sListStr = "$_ret_";
					vTypeStr.push_back("array('" + getSdpTypeName(stOperation.stRetType) + "', 0)");
				}
				for (unsigned i = 0; i < stOperation.vParam.size(); ++i)
				{
					const SdpInterface::Paramater &stParam = stOperation.vParam[i];
					if (stParam.bIsOut)
					{
						if (!sListStr.empty())
						{
							sListStr += ", ";
						}
						sListStr += "$" + stParam.sParamName;
						vTypeStr.push_back("array('" + getSdpTypeName(stParam.stParamType) + "', " + UtilString::tostr(i + 1) + ")");
					}
				}

				tab(2) << "list(" << sListStr << ") = stringToSdp($_stRspPacket_->sRspPayload, array(" << endl;
				for (unsigned i = 0; i < vTypeStr.size(); ++i)
				{
					tab(3) << vTypeStr[i] << "," << endl;
				}
				tab(2) << "));" << endl;

				if (bHasReturn)
				{
					tab(2) << "return $_ret_;" << endl;
				}
			}

			tab(1) << "}" << endl;
		}
		tab(0) << "}" << endl;
		tab(0) << endl;
	}
	string getStructFieldDefaultValue(const SdpStruct::StructField &stStructField, bool bIsForConstruct)
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
			if (bIsForConstruct)
			{
				sDefault = "array()";
			}
			else
			{
				sDefault = "null";
			}
			break;
		case SdpType_Struct:
			if (bIsForConstruct)
			{
				sDefault = "new \\" + UtilString::replace(stStructField.stFieldType.sFullName, "::", "\\") + "()";
			}
			else
			{
				sDefault = "null";
			}
			break;
		default:
			break;
		}
		return sDefault;
	}

private:
	ostringstream &tab(int n)
	{
		for (int i = 0; i < n; ++i)
		{
			os << "\t";
		}
		return os;
	}
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
				return "vector<" + s + ">";
			}
		case SdpType_Map:
			{
				string s1 = getSdpTypeName(stType.vInnerType[0]);
				string s2 = getSdpTypeName(stType.vInnerType[1]);
				return "map<" + s1 + ", " + s2 + ">";
			}
		case SdpType_Struct:
			{
				return "\\" + UtilString::replace(stType.sFullName, "::", "\\");
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
	bool				m_bFlatIncludeFilePath;
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

		Sdp2Php sdp2php(stTree);
		if (option.hasOption("dir"))
		{
			sdp2php.setOutputDir(option.getOption("dir"));
		}
		if (option.hasOption("flat-include-path"))
		{
			sdp2php.setFlatIncludeFilePath(UtilString::strto<int>(option.getOption("flat-include-path")) == 0 ? false : true);
		}
		sdp2php.generate();
	}
	return 0;
}
