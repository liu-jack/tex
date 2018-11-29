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

class Sdp2Cpp
{
public:
	explicit Sdp2Cpp(const SdpParseTree &stTree) : m_stTree(stTree), m_bIsRpc(false) {}

	void setOutputDir(const string &sDir) { m_sOutputDir = sDir; }
	void setRpcCall(bool bIsRpc) { m_bIsRpc = bIsRpc; }

	void generate()
	{
		string sHeaderFile = UtilFile::replaceFileExt(UtilFile::getFileBasename(m_stTree.sFileName), ".h", true);
		string sSouceFile = UtilFile::replaceFileExt(UtilFile::getFileBasename(m_stTree.sFileName), ".cpp", true);
		if (!m_sOutputDir.empty())
		{
			UtilFile::makeDirectoryRecursive(m_sOutputDir);
			sHeaderFile = m_sOutputDir + "/" + sHeaderFile;
			sSouceFile = m_sOutputDir + "/" + sSouceFile;
		}

		string sHeaderContent = generateHeader();
		UtilFile::saveToFile(sHeaderFile, sHeaderContent);

		if (m_stTree.hasInterface())
		{
			string sSourceContent = generateSource();
			UtilFile::saveToFile(sSouceFile, sSourceContent);
		}
	}

	string generateHeader()
	{
		os.str("");
		os.clear();
		printHead();
		printInclude();
		printNamespace();
		printTail();
		return os.str();
	}

	string generateSource()
	{
		os.str("");
		os.clear();
		printSourceInclude();
		printSourceNamespace();
		return os.str();
	}

private:
	void printHead()
	{
		string sDefine = "_SDP_" + UtilString::toupper(UtilFile::replaceFileExt(UtilFile::getFileBasename(m_stTree.sFileName), "_HPP", true)) + "_";
		os << "#ifndef " << sDefine << endl;
		os << "#define " << sDefine << endl;
		os << endl;
	}
	void printInclude()
	{
		os << "#include <string>" << endl;
		os << "#include <vector>" << endl;
		os << "#include <map>" << endl;
		os << "#include <algorithm>" << endl;
		os << "#include \"sdp/Sdp.h\"" << endl;
		if (m_stTree.hasInterface())
		{
			os << "#include \"service/Service.h\"" << endl;
		}
		os << "using namespace std;" << endl;
		os << "using namespace mfw;" << endl;
		for (unsigned i = 0; i < m_stTree.vInclude.size(); ++i)
		{
			os << "#include \"" << UtilFile::replaceFileExt(m_stTree.vInclude[i], ".h", true) << "\"" << endl;
		}
		os << endl;
	}
	void printTail()
	{
		os << endl;
		os << "#endif" << endl;
	}
	void printNamespace()
	{
		for (unsigned i = 0; i < m_stTree.vNamespace.size(); ++i)
		{
			const SdpNamespace &stNamespace = m_stTree.vNamespace[i];
			os << "namespace " << stNamespace.sNamespaceName << endl;
			os << "{" << endl;
			for (unsigned i = 0; i < stNamespace.vEnum.size(); ++i)
			{
				printEnum(stNamespace.vEnum[i]);
			}
			for (unsigned i = 0; i < stNamespace.vStruct.size(); ++i)
			{
				printStruct(stNamespace.vStruct[i]);
			}
			for (unsigned i = 0; i < stNamespace.vInterface.size(); ++i)
			{
				printInterface(stNamespace.vInterface[i]);
			}
			os << "}" << endl << endl;

			if (!stNamespace.vStruct.empty())
			{
				os << "namespace std" << endl;
				os << "{" << endl;

				for (unsigned i = 0; i < stNamespace.vStruct.size(); ++i)
				{
					const SdpStruct &stStruct = stNamespace.vStruct[i];
					tab(0) << "inline void swap(" << stNamespace.sNamespaceName << "::" << stStruct.sStructName << " &a, " << stNamespace.sNamespaceName << "::" << stStruct.sStructName << " &b) { a.swap(b); }" << endl;
				}

				os << "}" << endl;
			}

			/*
			for (unsigned i = 0; i < stNamespace.vStruct.size(); ++i)
			{
				const SdpStruct &stStruct = stNamespace.vStruct[i];
				printStructAssignment(stNamespace, stStruct);
			}
			*/
		}
	}
	void printStruct(const SdpStruct &stStruct)
	{
		tab(0) << "struct " << stStruct.sStructName << endl;
		tab(0) << "{" << endl;
		printStructMember(stStruct);
		printStructConstructor(stStruct);
		printStructName(stStruct);
		printStructVisitor(stStruct);
		printStructSwap(stStruct);
		printStructOperator(stStruct);
		tab(0) << "};" << endl << endl;
	}
	void printEnum(const SdpEnum &stEnum)
	{
		tab(0) << "enum " << stEnum.sEnumName << endl;
		tab(0) << "{" << endl;
		for (unsigned i = 0; i < stEnum.vEnumItem.size(); ++i)
		{
			const SdpEnum::EnumItem &stEnumItem = stEnum.vEnumItem[i];
			tab(1) << stEnumItem.sName;
			if (stEnumItem.bHasAssignValue)
			{
				os << " = " << getSdpAssignValue(stEnumItem.stAssignValue);
			}
			os << "," << endl;
		}
		tab(0) << "};" << endl;

		tab(0) << "static inline const char *etos(" << stEnum.sEnumName << " e)" << endl;
		tab(0) << "{" << endl;
		tab(1) << "switch (e)" << endl;
		tab(1) << "{" << endl;
		for (unsigned i = 0; i < stEnum.vEnumItem.size(); ++i)
		{
			const SdpEnum::EnumItem &stEnumItem = stEnum.vEnumItem[i];
			tab(2) << "case " << stEnumItem.sName << ": return \"" << stEnumItem.sName << "\";" << endl;
		}
		tab(2) << "default: return \"\";" << endl;
		tab(1) << "}" << endl;
		tab(0) << "}" << endl;

		tab(0) << "static inline bool stoe(const string &s, " << stEnum.sEnumName << " &e)" << endl;
		tab(0) << "{" << endl;
		for (unsigned i = 0; i < stEnum.vEnumItem.size(); ++i)
		{
			const SdpEnum::EnumItem &stEnumItem = stEnum.vEnumItem[i];
			tab(1) << "if (s == \"" << stEnumItem.sName << "\") { e = " << stEnumItem.sName << "; return true; }" << endl;
		}
		tab(1) << "return false;" << endl;
		tab(0) << "}" << endl << endl;
	}
	void printInterface(const SdpInterface &stInterface)
	{
		printInterfaceCallback(stInterface);
		printInterfaceProxy(stInterface);
		printInterfaceService(stInterface);
	}
	void printStructMember(const SdpStruct &stStruct)
	{
		for (unsigned i = 0; i < stStruct.vStructField.size(); ++i)
		{
			const SdpStruct::StructField &stStructField = stStruct.vStructField[i];
			tab(1) << getSdpTypeName(stStructField.stFieldType) << " " << stStructField.sFieldName << ";" << endl;
		}
		if (!stStruct.vStructField.empty())
		{
			os << endl;
		}
	}
	void printStructConstructor(const SdpStruct &stStruct)
	{
		tab(1) << stStruct.sStructName << "()";
		unsigned num = 0;
		for (unsigned i = 0; i < stStruct.vStructField.size(); ++i)
		{
			const SdpStruct::StructField &stStructField = stStruct.vStructField[i];
			string sDefault = getStructFieldDefaultValue(stStructField);
			if (sDefault.empty())
			{
				continue;
			}
			if (num == 0)
			{
				os << ": " << endl;
			}
			else
			{
				os << ", " << endl;
			}
			tab(2) << stStructField.sFieldName << "(" << sDefault << ")";
			++num;
		}
		if (num == 0)
		{
			os << " {}" << endl;
		}
		else
		{
			os << endl;
			tab(1) << "{" << endl;
			tab(1) << "}" << endl;
		}
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
		case SdpType_Int64:
		case SdpType_UInt64:
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
		case SdpType_String:
			{
				if (stStructField.bHasDefaultValue)
				{
					sDefault = getSdpAssignValue(stStructField.stDefaultValue);
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
					sDefault = "static_cast<" + stStructField.stFieldType.sFullName + ">(0)";
				}
			}
			break;
		case SdpType_Vector:
		case SdpType_Map:
		case SdpType_Struct:
		default:
			break;
		}
		return sDefault;
	}
	void printStructName(const SdpStruct &stStruct)
	{
		tab(1) << "const char *getName() const { return \"" << stStruct.sStructName << "\"; } " << endl;
	}
	void printStructVisitor(const SdpStruct &stStruct)
	{
		for (int i = 0; i < 2; ++i)
		{
			tab(1) << "template <typename T>" << endl;
			if (stStruct.vStructField.empty())
			{
				tab(1) << "void visit(T &, bool) " << (i == 0 ? "" : "const ") << endl;
				tab(1) << "{" << endl;
				tab(1) << "}" << endl;
			}
			else
			{
				tab(1) << "void visit(T &t, bool bOpt) " << (i == 0 ? "" : "const ") << endl;
				tab(1) << "{" << endl;
				tab(2) << "if (!bOpt)" << endl;
				tab(2) << "{" << endl;
				printStructVisitorCode(stStruct, false);
				tab(2) << "}" << endl;
				tab(2) << "else" << endl;
				tab(2) << "{" << endl;
				printStructVisitorCode(stStruct, true);
				tab(2) << "}" << endl;
				tab(1) << "}" << endl;
			}
		}
	}
	void printStructVisitorCode(const SdpStruct &stStruct, bool bOpt)
	{
		for (unsigned i = 0; i < stStruct.vStructField.size(); ++i)
		{
			const SdpStruct::StructField &stStructField = stStruct.vStructField[i];
			tab(3);
			if (!bOpt && !stStructField.bRequired && stStructField.stFieldType.iTypeId != SdpType_Struct)
			{
				os << "if (";
				string sDefault = getStructFieldDefaultValue(stStructField);
				if (!sDefault.empty())
				{
					os << stStructField.sFieldName << " != " << sDefault;
				}
				else
				{
					os << "!" << stStructField.sFieldName << ".empty()";
				}
				os << ") ";
			}
			os << "t.visit(" << stStructField.iTag << ", " << (stStructField.bRequired ? "true" : "false") << ", \"" << stStructField.sFieldName << "\", " << stStructField.sFieldName << ");" << endl;
		}
	}
	void printStructSwap(const SdpStruct &stStruct)
	{
		if (stStruct.vStructField.empty())
		{
			tab(1) << "void swap(" << stStruct.sStructName << " &)" << endl;
			tab(1) << "{" << endl;
			tab(1) << "}" << endl;
		}
		else
		{
			tab(1) << "void swap(" << stStruct.sStructName << " &b)" << endl;
			tab(1) << "{" << endl;
			tab(2) << "using std::swap;" << endl;
			for (unsigned i = 0; i < stStruct.vStructField.size(); ++i)
			{
				const SdpStruct::StructField &stStructField = stStruct.vStructField[i];
				tab(2) << "swap(" << stStructField.sFieldName << ", b." << stStructField.sFieldName << ");" << endl;
			}
			tab(1) << "}" << endl;
		}
	}
	void printStructOperator(const SdpStruct &stStruct)
	{
		// ==
		if (stStruct.vStructField.empty())
		{
			tab(1) << "bool operator== (const " << stStruct.sStructName << " &) const" << endl;
			tab(1) << "{" << endl;
			tab(2) << "return true;" << endl;
			tab(1) << "}" << endl;
		}
		else
		{
			tab(1) << "bool operator== (const " << stStruct.sStructName << " &rhs) const" << endl;
			tab(1) << "{" << endl;
			for (unsigned i = 0; i < stStruct.vStructField.size(); ++i)
			{
				const SdpStruct::StructField &stStructField = stStruct.vStructField[i];
				if (i == 0)
				{
					tab(2) << "return " << stStructField.sFieldName << " == rhs." << stStructField.sFieldName;
				}
				else
				{
					os << endl;
					tab(3) << "&& " << stStructField.sFieldName << " == rhs." << stStructField.sFieldName;
				}
			}
			os << ";" << endl;
			tab(1) << "}" << endl;
		}

		// !=
		tab(1) << "bool operator!= (const " << stStruct.sStructName << " &rhs) const" << endl;
		tab(1) << "{" << endl;
		tab(2) << "return !((*this) == rhs);" << endl;
		tab(1) << "}" << endl;

		if (!stStruct.vSortKey.empty())
		{
			// <
			tab(1) << "bool operator< (const " << stStruct.sStructName << " &rhs) const" << endl;
			tab(1) << "{" << endl;
			for (unsigned i = 0; i < stStruct.vSortKey.size(); ++i)
			{
				const string &sSortKey = stStruct.vSortKey[i];
				tab(2) << "if (" << sSortKey << " != rhs." << sSortKey << ") return " << sSortKey << " < rhs." << sSortKey << ";" << endl;
			}
			tab(2) << "return false;" << endl;
			tab(1) << "}" << endl;

			// >
			tab(1) << "bool operator> (const " << stStruct.sStructName << " &rhs) const" << endl;
			tab(1) << "{" << endl;
			tab(2) << "return rhs < (*this);" << endl;
			tab(1) << "}" << endl;
		}
	}
	void printStructAssignment(const SdpNamespace &stNamespace, const SdpStruct &stStruct)
	{
		tab(0) << "template <typename T>" << endl;
		tab(0) << "struct sdp_assign_from_imp <" << stNamespace.sNamespaceName << "::" << stStruct.sStructName << ", T>" << endl;
		tab(0) << "{" << endl;
		tab(1) << "sdp_assign_from_imp(" << stNamespace.sNamespaceName << "::" << stStruct.sStructName << " &a, const T &b)" << endl;
		tab(1) << "{" << endl;
		for (unsigned i = 0; i < stStruct.vStructField.size(); ++i)
		{
			const SdpStruct::StructField &stStructField = stStruct.vStructField[i];
			tab(2) << "sdp_assign_from(a." << stStructField.sFieldName << ", b." << stStructField.sFieldName << ");" << endl;
		}
		tab(1) << "}" << endl;
		tab(0) << "};" << endl;

		tab(0) << "template <typename T>" << endl;
		tab(0) << "struct sdp_assign_to_imp <T, " << stNamespace.sNamespaceName << "::" << stStruct.sStructName << ">" << endl;
		tab(0) << "{" << endl;
		tab(1) << "sdp_assign_to_imp(T &a, const " << stNamespace.sNamespaceName << "::" << stStruct.sStructName << " &b)" << endl;
		tab(1) << "{" << endl;
		for (unsigned i = 0; i < stStruct.vStructField.size(); ++i)
		{
			const SdpStruct::StructField &stStructField = stStruct.vStructField[i];
			tab(2) << "sdp_assign_to(a." << stStructField.sFieldName << ", b." << stStructField.sFieldName << ");" << endl;
		}
		tab(1) << "}" << endl;
		tab(0) << "};" << endl;
	}
	void printInterfaceCallback(const SdpInterface &stInterface)
	{
		tab(0) << "class " << stInterface.sInterfaceName << "PrxCallback: public mfw::ServiceProxyCallback" << endl;
		tab(0) << "{" << endl;
		tab(0) << "public:" << endl;
		tab(1) << "tr1::shared_ptr<" << stInterface.sInterfaceName << "PrxCallback> shared_from_this() { return tr1::static_pointer_cast<" << stInterface.sInterfaceName << "PrxCallback>(tr1::enable_shared_from_this<ServiceProxyCallback>::shared_from_this()); }" << endl << endl;
		for (unsigned i = 0; i < stInterface.vOperation.size(); ++i)
		{
			const SdpInterface::Operation &stOperation = stInterface.vOperation[i];
			tab(1) << "virtual void callback_" << stOperation.sOperationName << "(" << getOperationParam_async_rsp(stOperation, true) << ")" << endl;
			tab(1) << "{ throw std::runtime_error(\"callback_" << stOperation.sOperationName << "() overloading incorrect.\"); }" << endl;

			tab(1) << "virtual void callback_" << stOperation.sOperationName << "_exception(int32_t /*ret*/)" << endl;
			tab(1) << "{ throw std::runtime_error(\"callback_" << stOperation.sOperationName << "_exception() overloading incorrect.\"); }" << endl << endl;
		}
		tab(1) << "virtual void onDispatch(mfw::ReqMessage *msg);" << endl;
		tab(0) << "};" << endl;
		tab(0) << "typedef tr1::shared_ptr<" << stInterface.sInterfaceName << "PrxCallback> " << stInterface.sInterfaceName << "PrxCallbackPtr;" << endl;
		tab(0) << endl;
	}
	void printInterfaceProxy(const SdpInterface &stInterface)
	{
		tab(0) << "class " << stInterface.sInterfaceName << "Proxy : public mfw::ServiceProxy" << endl;
		tab(0) << "{" << endl;
		tab(0) << "public:" << endl;
		for (unsigned i = 0; i < stInterface.vOperation.size(); ++i)
		{
			const SdpInterface::Operation &stOperation = stInterface.vOperation[i];
			tab(1) << getSdpTypeName(stOperation.stRetType) << " " << stOperation.sOperationName
					<< "(" << getOperationParam_sync(stOperation) << ");" << endl;

			tab(1) << "void async_" << stOperation.sOperationName
					<< "(const " << stInterface.sInterfaceName << "PrxCallbackPtr &callback" << padParamFront(getOperationParam_async_req(stOperation)) << ");" << endl;
		}
		tab(0) << "};" << endl;
		tab(0) << "typedef tr1::shared_ptr<" << stInterface.sInterfaceName << "Proxy> " << stInterface.sInterfaceName << "Prx;" << endl;
		tab(0) << endl;
	}
	void printInterfaceService(const SdpInterface &stInterface)
	{
		if (m_bIsRpc)
		{
			return;
		}

		tab(0) << "class " << stInterface.sInterfaceName << " : public mfw::Service" << endl;
		tab(0) << "{" << endl;
		tab(0) << "public:" << endl;
		tab(1) << "virtual int handleMfwRequest(string &sRspPayload);" << endl;
		for (unsigned i = 0; i < stInterface.vOperation.size(); ++i)
		{
			const SdpInterface::Operation &stOperation = stInterface.vOperation[i];
			tab(1) << "virtual "<< getSdpTypeName(stOperation.stRetType) << " " << stOperation.sOperationName
					<< "(" << getOperationParam_sync(stOperation) << ") = 0;" << endl;
		}
		for (unsigned i = 0; i < stInterface.vOperation.size(); ++i)
		{
			const SdpInterface::Operation &stOperation = stInterface.vOperation[i];
			tab(1) << "static void async_response_" << stOperation.sOperationName
					<< "(const mfw::SdpCurrentPtr &current" << padParamFront(getOperationParam_async_rsp(stOperation)) << ");" << endl;
		}
		tab(0) << "};" << endl;
		tab(0) << endl;
	}

	void printSourceInclude()
	{
		os << "#include \"" << UtilFile::replaceFileExt(UtilFile::getFileBasename(m_stTree.sFileName), ".h", true) << "\"" << endl;
		os << endl;
	}
	void printSourceNamespace()
	{
		for (unsigned i = 0; i < m_stTree.vNamespace.size(); ++i)
		{
			const SdpNamespace &stNamespace = m_stTree.vNamespace[i];
			os << "namespace " << stNamespace.sNamespaceName << endl;
			os << "{" << endl;
			for (unsigned i = 0; i < stNamespace.vInterface.size(); ++i)
			{
				printSourceInterfaceLookup(stNamespace.vInterface[i]);
				printSourceInterfaceCallback(stNamespace.vInterface[i]);
				printSourceInterfaceProxy(stNamespace.vInterface[i]);
				printSourceInterfaceService(stNamespace.vInterface[i]);
			}
			os << "}" << endl;
		}
	}
	void printSourceInterfaceLookup(const SdpInterface &stInterface)
	{
		tab(0) << "static map<string, int> __init" << stInterface.sInterfaceName << "InterfaceLookup()" << endl;
		tab(0) << "{" << endl;
		tab(1) << "map<string, int> m;" << endl;
		for (unsigned i = 0; i < stInterface.vOperation.size(); ++i)
		{
			const SdpInterface::Operation &stOperation = stInterface.vOperation[i];
			tab(1) << "m[\"" << stOperation.sOperationName << "\"] = " << i << ";" << endl;
		}
		tab(1) << "return m;" << endl;
		tab(0) << "}" << endl;
		tab(0) << "static const map<string, int> g_m" << stInterface.sInterfaceName << "InterfaceLookup = __init" << stInterface.sInterfaceName << "InterfaceLookup();" << endl;
		tab(0) << endl;
	}
	void printSourceInterfaceCallback(const SdpInterface &stInterface)
	{
		tab(0) << "void " << stInterface.sInterfaceName << "PrxCallback::onDispatch(mfw::ReqMessage *msg)" << endl;
		tab(0) << "{" << endl;
		tab(1) << "map<string, int>::const_iterator it = g_m" << stInterface.sInterfaceName << "InterfaceLookup.find(msg->request.sFuncName);" << endl;
		tab(1) << "if (it == g_m" << stInterface.sInterfaceName << "InterfaceLookup.end()) throw std::runtime_error(\"proxy dispatch no such func\");" << endl;
		tab(1) << "switch (it->second)" << endl;
		tab(1) << "{" << endl;
		for (unsigned i = 0; i < stInterface.vOperation.size(); ++i)
		{
			const SdpInterface::Operation &stOperation = stInterface.vOperation[i];
			tab(1) << "case " << i << ":" << endl;
			tab(2) << "{" << endl;

			tab(3) << "if (msg->response.iMfwRet != 0)" << endl;
			tab(3) << "{" << endl;
			tab(4) << "callback_" << stOperation.sOperationName << "_exception(msg->response.iMfwRet);" << endl;
			tab(4) << "return;" << endl;
			tab(3) << "}" << endl;

			if (!m_bIsRpc)
			{
				string sInvokeParam;
				tab(3) << "SdpUnpacker unpacker(msg->response.sRspPayload);" << endl;
				if (stOperation.stRetType.iTypeId != SdpType_Void)
				{
					string sInitializer = getSdpTypeDefaultInitializer(stOperation.stRetType);
					if (!sInitializer.empty())
					{
						sInitializer = " = " + sInitializer;
					}
					tab(3) << getSdpTypeName(stOperation.stRetType) << " ret" << sInitializer << ";" << endl;
					tab(3) << "unpacker.unpack(0, true, \"ret\", ret);" << endl;
					sInvokeParam = "ret";
				}
				for (unsigned i = 0; i < stOperation.vParam.size(); ++i)
				{
					const SdpInterface::Paramater &stParam = stOperation.vParam[i];
					if (!stParam.bIsOut)
					{
						continue;
					}
					tab(3) << getSdpTypeName(stParam.stParamType) << " " << stParam.sParamName <<";" << endl;
					tab(3) << "unpacker.unpack(" << (i + 1) << ", true, \"" << stParam.sParamName << "\", " << stParam.sParamName << ");" << endl;
					if (!sInvokeParam.empty())
					{
						sInvokeParam += ", ";
					}
					sInvokeParam += stParam.sParamName;
				}
				tab(3) << "CallbackThreadData *pCbtd = CallbackThreadData::getData();" << endl;
				tab(3) << "pCbtd->setResponseContext(msg->response.context);" << endl;
				tab(3) << "callback_" << stOperation.sOperationName << "(" << sInvokeParam << ");" << endl;
				tab(3) << "pCbtd->delResponseContext();" << endl;
			}
			else
			{
				tab(3) << "// TODO" << endl;
				string sInvokeParam = "-1";
				for (unsigned i = 0; i < stOperation.vParam.size(); ++i)
				{
					const SdpInterface::Paramater &stParam = stOperation.vParam[i];
					if (!stParam.bIsOut)
					{
						continue;
					}
					tab(3) << "// " << getSdpTypeName(stParam.stParamType) << " " << stParam.sParamName <<";" << endl;
					if (!sInvokeParam.empty())
					{
						sInvokeParam += ", ";
					}
					sInvokeParam += stParam.sParamName;
				}
				tab(3) << "// callback_" << stOperation.sOperationName << "(" << sInvokeParam << ");" << endl;
			}
			tab(3) << "return;" << endl;

			tab(2) << "}" << endl;
		}
		tab(1) << "}" << endl;
		tab(1) << "throw std::runtime_error(\"proxy dispatch no such func\");" << endl;
		tab(0) << "}" << endl;
	}
	void printSourceInterfaceProxy(const SdpInterface &stInterface)
	{
		for (unsigned i = 0; i < stInterface.vOperation.size(); ++i)
		{
			const SdpInterface::Operation &stOperation = stInterface.vOperation[i];
			// sync call
			tab(0) << getSdpTypeName(stOperation.stRetType) << " " << stInterface.sInterfaceName << "Proxy::" << stOperation.sOperationName
					<< "(" << getOperationParam_sync(stOperation) << ")" << endl;
			tab(0) << "{" << endl;

			if (!m_bIsRpc)
			{
				tab(1) << "SdpPacker packer;" << endl;
				for (unsigned i = 0; i < stOperation.vParam.size(); ++i)
				{
					const SdpInterface::Paramater &stParam = stOperation.vParam[i];
					if (!stParam.bIsOut)
					{
						tab(1) << "packer.pack(" << (i + 1) << ", " << stParam.sParamName << ");" << endl;
					}
				}
				tab(1) << "mfw::ResponsePacket rsp;" << endl;
				tab(1) << "mfw_invoke(\"" << stOperation.sOperationName << "\", packer.getData(), rsp);" << endl;
				tab(1) << "SdpUnpacker unpacker(rsp.sRspPayload);" << endl;
				if (stOperation.stRetType.iTypeId != SdpType_Void)
				{
					string sInitializer = getSdpTypeDefaultInitializer(stOperation.stRetType);
					if (!sInitializer.empty())
					{
						sInitializer = " = " + sInitializer;
					}
					tab(1) << getSdpTypeName(stOperation.stRetType) << " ret" << sInitializer << ";" << endl;
					tab(1) << "unpacker.unpack(0, true, \"ret\", ret);" << endl;
				}
				for (unsigned i = 0; i < stOperation.vParam.size(); ++i)
				{
					const SdpInterface::Paramater &stParam = stOperation.vParam[i];
					if (stParam.bIsOut)
					{
						tab(1) << "unpacker.unpack(" << (i + 1) << ", true, \"" << stParam.sParamName << "\", " << stParam.sParamName << ");" << endl;
					}
				}
				if (stOperation.stRetType.iTypeId != SdpType_Void)
				{
					tab(1) << "return ret;" << endl;
				}
			}
			else
			{
				tab(1) << "// TODO" << endl;
				for (unsigned i = 0; i < stOperation.vParam.size(); ++i)
				{
					const SdpInterface::Paramater &stParam = stOperation.vParam[i];
					tab(1) << "(void)" << stParam.sParamName << ";" << endl;
				}
				tab(1) << "mfw::ResponsePacket rsp;" << endl;
				tab(1) << "rpc_call(0, \"" << stOperation.sOperationName << "\", \"\", rsp);" << endl;
				tab(1) << "return 0;" << endl;
			}
			tab(0) << "}" << endl;
			tab(0) << endl;

			// async call
			tab(0) << "void " << stInterface.sInterfaceName << "Proxy::async_" << stOperation.sOperationName
					<< "(const " << stInterface.sInterfaceName << "PrxCallbackPtr &callback" << padParamFront(getOperationParam_async_req(stOperation)) << ")" << endl;
			tab(0) << "{" << endl;
			if (!m_bIsRpc)
			{
				tab(1) << "SdpPacker packer;" << endl;
				for (unsigned i = 0; i < stOperation.vParam.size(); ++i)
				{
					const SdpInterface::Paramater &stParam = stOperation.vParam[i];
					if (!stParam.bIsOut)
					{
						tab(1) << "packer.pack(" << (i + 1) << ", " << stParam.sParamName << ");" << endl;
					}
				}
				tab(1) << "mfw_invoke_async(\"" << stOperation.sOperationName << "\", packer.getData(), callback);" << endl;
			}
			else
			{
				tab(1) << "// TODO" << endl;
				for (unsigned i = 0; i < stOperation.vParam.size(); ++i)
				{
					const SdpInterface::Paramater &stParam = stOperation.vParam[i];
					if (!stParam.bIsOut)
					{
						tab(1) << "(void)" << stParam.sParamName << ";" << endl;
					}
				}
				tab(1) << "rpc_call_async(0, \"" << stOperation.sOperationName << "\", \"\", callback);" << endl;
			}
			tab(0) << "}" << endl;
			tab(0) << endl;
		}
	}
	void printSourceInterfaceService(const SdpInterface &stInterface)
	{
		if (m_bIsRpc)
		{
			return;
		}

		for (unsigned i = 0; i < stInterface.vOperation.size(); ++i)
		{
			const SdpInterface::Operation &stOperation = stInterface.vOperation[i];
			tab(0) << "void " << stInterface.sInterfaceName << "::async_response_" << stOperation.sOperationName
					<< "(const mfw::SdpCurrentPtr &current" << padParamFront(getOperationParam_async_rsp(stOperation)) << ")" << endl;
			tab(0) << "{" << endl;
			tab(1) << "SdpPacker packer;" << endl;
			if (stOperation.stRetType.iTypeId != SdpType_Void)
			{
				tab(1) << "packer.pack(0, ret);" << endl;
			}
			for (unsigned i = 0; i < stOperation.vParam.size(); ++i)
			{
				const SdpInterface::Paramater &stParam = stOperation.vParam[i];
				if (stParam.bIsOut)
				{
					tab(1) << "packer.pack(" << (i + 1) << ", " << stParam.sParamName << ");" << endl;
				}
			}
	    	tab(1) << "current->sendMfwResponse(0, packer.getData());" << endl;
			tab(0) << "}" << endl;
			tab(0) << endl;
		}

		if (stInterface.vOperation.empty())
		{
			tab(0) << "int " << stInterface.sInterfaceName << "::handleMfwRequest(string &/*sRspPayload*/)" << endl;
		}
		else
		{
			tab(0) << "int " << stInterface.sInterfaceName << "::handleMfwRequest(string &sRspPayload)" << endl;
		}

		tab(0) << "{" << endl;
		tab(1) << "mfw::SdpCurrentPtr &current = getCurrent();" << endl;
		tab(1) << "map<string, int>::const_iterator it = g_m" << stInterface.sInterfaceName << "InterfaceLookup.find(current->getFuncName());" << endl;
		tab(1) << "if (it == g_m" << stInterface.sInterfaceName << "InterfaceLookup.end()) return mfw::SDPSERVERNOFUNCERR;" << endl;
		tab(1) << "switch (it->second)" << endl;
		tab(1) << "{" << endl;
		for (unsigned i = 0; i < stInterface.vOperation.size(); ++i)
		{
			const SdpInterface::Operation &stOperation = stInterface.vOperation[i];
			tab(1) << "case " << i << ":" << endl;
			tab(2) << "{" << endl;

			tab(3) << "SdpUnpacker unpacker(current->getRequestBuffer());" << endl;
			for (unsigned i = 0; i < stOperation.vParam.size(); ++i)
			{
				const SdpInterface::Paramater &stParam = stOperation.vParam[i];
				if (!stParam.bIsOut)
				{
					tab(3) << getSdpTypeName(stParam.stParamType) << " " << stParam.sParamName <<";" << endl;
					tab(3) << "unpacker.unpack(" << (i + 1) << ", true, \"" << stParam.sParamName << "\", " << stParam.sParamName << ");" << endl;
				}
			}

			string sInvokeParam;
			for (unsigned i = 0; i < stOperation.vParam.size(); ++i)
			{
				const SdpInterface::Paramater &stParam = stOperation.vParam[i];
				if (stParam.bIsOut)
				{
					tab(3) << getSdpTypeName(stParam.stParamType) << " " << stParam.sParamName <<";" << endl;
				}
				if (!sInvokeParam.empty())
				{
					sInvokeParam += ", ";
				}
				sInvokeParam += stParam.sParamName;
			}
			if (stOperation.stRetType.iTypeId != SdpType_Void)
			{
				tab(3) << getSdpTypeName(stOperation.stRetType) << " ret = " << stOperation.sOperationName << "(" << sInvokeParam << ");" << endl;
			}
			else
			{
				tab(3) << stOperation.sOperationName << "(" << sInvokeParam << ");" << endl;
			}
			tab(3) << "if (current->isResponse())" << endl;
			tab(3) << "{" << endl;
			tab(4) << "SdpPacker packer;" << endl;
			if (stOperation.stRetType.iTypeId != SdpType_Void)
			{
				tab(4) << "packer.pack(0, ret);" << endl;
			}
			for (unsigned i = 0; i < stOperation.vParam.size(); ++i)
			{
				const SdpInterface::Paramater &stParam = stOperation.vParam[i];
				if (stParam.bIsOut)
				{
					tab(4) << "packer.pack(" << (i + 1) << ", " << stParam.sParamName << ");" << endl;
				}
			}
			tab(4) << "swap(sRspPayload, packer.getData());" << endl;
			tab(3) << "}" << endl;
			tab(3) << "return 0;" << endl;
			tab(2) << "}" << endl;
		}
		tab(1) << "}" << endl;
		tab(1) << "return mfw::SDPSERVERNOFUNCERR;" << endl;
		tab(0) << "}" << endl;
		tab(0) << endl;
	}

	string getOperationParam_sync(const SdpInterface::Operation &stOperation)
	{
		string s;
		for (unsigned i = 0; i < stOperation.vParam.size(); ++i)
		{
			const SdpInterface::Paramater &stParam = stOperation.vParam[i];
			if (!s.empty())
			{
				s += ", ";
			}
			if (stParam.bIsOut)
			{
				s += getSdpInterfaceOutputParam(stParam.stParamType, stParam.sParamName);
			}
			else
			{
				s += getSdpInterfaceInputParam(stParam.stParamType, stParam.sParamName);
			}
		}
		return s;
	}
	string getOperationParam_async_req(const SdpInterface::Operation &stOperation)
	{
		string s;
		for (unsigned i = 0; i < stOperation.vParam.size(); ++i)
		{
			const SdpInterface::Paramater &stParam = stOperation.vParam[i];
			if (stParam.bIsOut)
			{
				continue;
			}
			if (!s.empty())
			{
				s += ", ";
			}
			s += getSdpInterfaceInputParam(stParam.stParamType, stParam.sParamName);
		}
		return s;
	}
	string getOperationParam_async_rsp(const SdpInterface::Operation &stOperation, bool bCommentedParam = false)
	{
		string s;
		if (stOperation.stRetType.iTypeId != SdpType_Void)
		{
			s += getSdpInterfaceInputParam(stOperation.stRetType, bCommentedParam ? "/*ret*/" : "ret");
		}
		for (unsigned i = 0; i < stOperation.vParam.size(); ++i)
		{
			const SdpInterface::Paramater &stParam = stOperation.vParam[i];
			if (!stParam.bIsOut)
			{
				continue;
			}
			if (!s.empty())
			{
				s += ", ";
			}
			s += getSdpInterfaceInputParam(stParam.stParamType, bCommentedParam ? ("/*" + stParam.sParamName + "*/") : stParam.sParamName);
		}
		return s;
	}
	string padParamFront(const string &s)
	{
		return s.empty() ? s : ", " + s;
	}
	string padParamBack(const string &s)
	{
		return s.empty() ? s : s + ", ";
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
		case SdpType_Struct: return stType.sFullName;
		}
		return "";
	}

	static string getSdpTypeDefaultInitializer(const SdpType &stType)
	{
		switch (stType.iTypeId)
		{
		case SdpType_Void: return "";
		case SdpType_Bool: return "false";
		case SdpType_Char:
		case SdpType_Int8:
		case SdpType_UInt8:
		case SdpType_Int16:
		case SdpType_UInt16:
		case SdpType_Int32:
		case SdpType_UInt32:
		case SdpType_Int64:
		case SdpType_UInt64:
			return "0";
		case SdpType_Float:
		case SdpType_Double:
			return "0.0";
		case SdpType_String:
		case SdpType_Vector:
		case SdpType_Map:
		case SdpType_Enum:
		case SdpType_Struct:
			return "";
		}
		return "";
	}

	static string getSdpInterfaceInputParam(const SdpType &stType, const string &sParamName)
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
			return getSdpTypeName(stType) + " " + sParamName;
		case SdpType_String:
		case SdpType_Vector:
		case SdpType_Map:
		case SdpType_Enum:
		case SdpType_Struct:
			return "const " + getSdpTypeName(stType) + " &" + sParamName;
		}
		return "";
	}

	static string getSdpInterfaceOutputParam(const SdpType &stType, const string &sParamName)
	{
		return getSdpTypeName(stType) + " &" + sParamName;
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
	bool				m_bIsRpc;
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
			cerr << "fail to parse file: " << option.getParam()[i] << endl;
			continue;
		}

		//cout << dumpsdp(stTree) << endl;

		Sdp2Cpp sdp2cpp(stTree);
		if (option.hasOption("dir"))
		{
			sdp2cpp.setOutputDir(option.getOption("dir"));
		}
		if (option.hasOption("rpc"))
		{
			sdp2cpp.setRpcCall(true);
		}
		sdp2cpp.generate();
	}
	return 0;
}
