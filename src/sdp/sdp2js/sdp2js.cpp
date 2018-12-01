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

class Sdp2Js
{
public:
    explicit Sdp2Js(const SdpParseTree &stTree) : m_stTree(stTree) {}

    void setOutputDir(const string &sDir)
    {
        m_sOutputDir = sDir;
    }

    void generate()
    {
        string sSouceFile = UtilFile::replaceFileExt(UtilFile::getFileBasename(m_stTree.sFileName), ".js", true);
        if (!m_sOutputDir.empty()) {
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
        os << "var sdp = require('sdp');" << endl;
        os << "var Long = require('long');" << endl;

        for (unsigned i = 0; i < m_stTree.vInclude.size(); ++i) {
            string sModule = UtilFile::replaceFileExt(UtilFile::getFileBasename(m_stTree.vInclude[i]), "", true);
            os << "require('" << sModule << "');" << endl;
        }
        os << endl;
    }
    void printNamespace()
    {
        for (unsigned i = 0; i < m_stTree.vNamespace.size(); ++i) {
            const SdpNamespace &stNamespace = m_stTree.vNamespace[i];
            os << "var " << stNamespace.sNamespaceName << " = window." << stNamespace.sNamespaceName << " || {};" << endl;
            os << "window." << stNamespace.sNamespaceName << " = " << stNamespace.sNamespaceName << ";" << endl;
            os << endl;

            for (unsigned i = 0; i < stNamespace.vEnum.size(); ++i) {
                printEnum(stNamespace.sNamespaceName, stNamespace.vEnum[i]);
            }
            for (unsigned i = 0; i < stNamespace.vStruct.size(); ++i) {
                printStruct(stNamespace.sNamespaceName, stNamespace.vStruct[i]);
            }
        }
    }
    void printEnum(const string &sModule, const SdpEnum &stEnum)
    {
        for (unsigned i = 0; i < stEnum.vEnumItem.size(); ++i) {
            const SdpEnum::EnumItem &stEnumItem = stEnum.vEnumItem[i];
            os << sModule << "." << stEnumItem.sName << " = ";
            if (stEnumItem.bHasAssignValue) {
                os << getSdpAssignValue(stEnumItem.stAssignValue);
            } else {
                if (i == 0) {
                    os << 0;
                } else {
                    os << sModule << "." << stEnum.vEnumItem[i - 1].sName << " + 1";
                }
            }
            os << ";" << endl;
        }
        if (!stEnum.vEnumItem.empty()) {
            os << endl;
        }
    }
    void printStruct(const string &sModule, const SdpStruct &stStruct)
    {
        // 构造函数
        tab(0) << sModule << "." << stStruct.sStructName << " = function() {" << endl;
        for (uint32_t i = 0; i < stStruct.vStructField.size(); ++i) {
            tab(1) << "this." << stStruct.vStructField[i].sFieldName << " = " << getStructFieldDefaultValue(stStruct.vStructField[i]) << ";" << endl;
        }
        os << "};" << endl;

        tab(0) << sModule << "." << stStruct.sStructName << "._classname = '" << sModule+"."+stStruct.sStructName << "';" << endl;
        tab(0) << sModule << "." << stStruct.sStructName << ".prototype._classname = '" << sModule+"."+stStruct.sStructName << "';" << endl;

        tab(0) << sModule << "." << stStruct.sStructName << "." << "_write = function(os, tag, val) {" << endl;
        tab(1) << "os.writeStruct(tag, true, val);" << endl;
        tab(0) << "};" << endl;
        tab(0) << endl;
        tab(0) << sModule << "." << stStruct.sStructName << "." << "_read = function(is, tag, def) {" << endl;
        tab(1) << "return is.readStruct(tag, true, def);" << endl;
        tab(0) << "};" << endl;
        tab(0) << endl;

        tab(0) << sModule << "." << stStruct.sStructName << "." << "_readFrom = function(is) {" << endl;
        tab(1) << "var ret = new " << sModule << "." << stStruct.sStructName << ";" << endl;
        for (uint32_t i = 0; i < stStruct.vStructField.size(); ++i) {
            tab(1) << "ret." << stStruct.vStructField[i].sFieldName << " = " << "is.read" << getSdpFieldFunName(stStruct.vStructField[i].stFieldType) << "(" << stStruct.vStructField[i].iTag << ", " << (stStruct.vStructField[i].bRequired?"true":"false") << ", " << getStructFieldDefaultValue(stStruct.vStructField[i], false) << ");" << endl;
        }
        tab(1) << "return ret;" << endl;
        tab(0) << "};" << endl;
        tab(0) << endl;

        tab(0) << sModule << "." << stStruct.sStructName << ".prototype." << "_writeTo = function(os) {" << endl;
        for (uint32_t i = 0; i < stStruct.vStructField.size(); ++i) {
            if (!stStruct.vStructField[i].bRequired) {
                if (stStruct.vStructField[i].stFieldType.iTypeId == SdpType_Vector) {
                    tab(1) << "if (this." << stStruct.vStructField[i].sFieldName << ".length != 0) ";
                } else if (stStruct.vStructField[i].stFieldType.iTypeId == SdpType_Map) {
                    tab(1) << "if (this." << stStruct.vStructField[i].sFieldName << ".size() != 0) ";
                } else if (stStruct.vStructField[i].stFieldType.iTypeId != SdpType_Struct) {
                    string sDefault = getStructFieldDefaultValue(stStruct.vStructField[i], false);
                    tab(1) << "if (this." << stStruct.vStructField[i].sFieldName << " != " << sDefault << ") ";
                } else {
                    tab(1);
                }
            } else {
                tab(1);
            }
            tab(0) << "os.write" << getSdpFieldFunName(stStruct.vStructField[i].stFieldType) << "(" << stStruct.vStructField[i].iTag << ", " << (stStruct.vStructField[i].bRequired?"true":"false")  << ", " << "this." << stStruct.vStructField[i].sFieldName << ");" << endl;
        }
        tab(0) << "};" << endl;
    }

    string getStructFieldDefaultValue(const SdpStruct::StructField &stStructField, bool constructor=true)
    {
        string sDefault;
        switch (stStructField.stFieldType.iTypeId) {
        case SdpType_Bool: {
            if (stStructField.bHasDefaultValue) {
                sDefault = getSdpAssignValue(stStructField.stDefaultValue);
            } else {
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
        case SdpType_Double: {
            if (stStructField.bHasDefaultValue) {
                sDefault = getSdpAssignValue(stStructField.stDefaultValue);
            } else {
                sDefault = "0";
            }
        }
        break;
        case SdpType_Int64: {
            if (stStructField.bHasDefaultValue) {
                sDefault = getSdpAssignValue(stStructField.stDefaultValue);
            } else {
                sDefault = "Long.ZERO";
            }
        }
        break;
        case SdpType_UInt64: {
            if (stStructField.bHasDefaultValue) {
                sDefault = getSdpAssignValue(stStructField.stDefaultValue);
            } else {
                sDefault = "Long.UZERO";
            }
        }
        break;
        case SdpType_String: {
            if (stStructField.bHasDefaultValue) {
                sDefault = getSdpAssignValue(stStructField.stDefaultValue);
            } else {
                sDefault = "''";
            }
        }
        break;
        case SdpType_Enum: {
            if (stStructField.bHasDefaultValue) {
                sDefault = getSdpAssignValue(stStructField.stDefaultValue);
            } else {
                sDefault = "0";
            }
        }
        break;
        case SdpType_Vector: {
            string s = getSdpTypeName(stStructField.stFieldType.vInnerType[0]);
            if (constructor) sDefault = "new ";
            if (s == "sdp.Char") sDefault += "sdp.Buffer";
            else sDefault += "sdp.Vector(" + s + ")";
        }
        break;
        case SdpType_Map: {
            string s1 = getSdpTypeName(stStructField.stFieldType.vInnerType[0]);
            string s2 = getSdpTypeName(stStructField.stFieldType.vInnerType[1]);
            if (constructor) sDefault = "new ";
            sDefault += "sdp.Map(" + s1 + "," + s2 + ")";
        }
        break;
        case SdpType_Struct: {
            if (constructor) sDefault = "new ";
            sDefault += "" + UtilString::replace(stStructField.stFieldType.sFullName, "::", ".");
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
        for (int i = 0; i < n; ++i) {
            os << "\t";
        }
        return os;
    }
    static string getSdpTypeName(const SdpType &stType)
    {
        switch (stType.iTypeId) {
        case SdpType_Bool:
            return "sdp.Boolean";
        case SdpType_Char:
            return "sdp.Char";
        case SdpType_Int8:
            return "sdp.Int8";
        case SdpType_UInt8:
            return "sdp.Uint8";
        case SdpType_Int16:
            return "sdp.Int16";
        case SdpType_UInt16:
            return "sdp.Uint16";
        case SdpType_Int32:
            return "sdp.Int32";
        case SdpType_UInt32:
            return "sdp.Uint32";
        case SdpType_Int64:
            return "sdp.Int64";
        case SdpType_UInt64:
            return "sdp.Uint64";
        case SdpType_Float:
            return "sdp.Float";
        case SdpType_Double:
            return "sdp.Double";
        case SdpType_String:
            return "sdp.String";
        case SdpType_Vector: {
            string s = getSdpTypeName(stType.vInnerType[0]);
            if (s == "sdp.Char") return "sdp.Buffer";
            return "sdp.Vector(" + s + ")";
        }
        case SdpType_Map: {
            string s1 = getSdpTypeName(stType.vInnerType[0]);
            string s2 = getSdpTypeName(stType.vInnerType[1]);
            return "sdp.Map(" + s1 + ", " + s2 + ")";
        }
        case SdpType_Struct:
            return UtilString::replace(stType.sFullName, "::", ".");
        }
        return "";
    }

    static string getSdpAssignValue(const SdpAssignValue &stValue)
    {
        switch (stValue.iType) {
        case SdpAssignValue::ValueType_Boolean:
            return stValue.bBoolean ? "true" : "false";
        case SdpAssignValue::ValueType_Integer:
            return stValue.sValue;
        case SdpAssignValue::ValueType_Float:
            return stValue.sValue;
        case SdpAssignValue::ValueType_String:
            return stValue.sValue.empty() ? "" : "\"" + stValue.sValue + "\"";
        }
        return "";
    }

    static string getSdpFieldFunName(const SdpType &stType)
    {
        switch (stType.iTypeId) {
        case SdpType_Bool:
            return "Boolean";
        case SdpType_Char:
            return "Uint8";
        case SdpType_Int8:
            return "Int8";
        case SdpType_UInt8:
            return "Uint8";
        case SdpType_Int16:
            return "Int16";
        case SdpType_UInt16:
            return "Uint16";
        case SdpType_Int32:
            return "Int32";
        case SdpType_UInt32:
            return "Uint32";
        case SdpType_Int64:
            return "Int64";
        case SdpType_UInt64:
            return "Uint64";
        case SdpType_Float:
            return "Float";
        case SdpType_Double:
            return "Double";
        case SdpType_String:
            return "String";
        case SdpType_Vector: {
            string s1 = getSdpTypeName(stType.vInnerType[0]);
            if (s1 != "sdp.Char") return "Vector";
            else return "Bytes";
        }
        case SdpType_Map:
            return "Map";
        case SdpType_Struct:
            return "Struct";
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
    if (option.getParam().empty()) {
        cerr << "Usage: " << argv[0] << " [--dir=] sdpfile" << endl;
        return -1;
    }

    for (unsigned i = 0; i < option.getParam().size(); ++i) {
        SdpParseTree stTree;
        int ret = parsesdp(option.getParam()[i], stTree);
        if (ret != 0) {
            cerr << "fail to parse file: " << argv[i] << endl;
            continue;
        }

        //cout << dumpsdp(stTree) << endl;

        Sdp2Js sdp2js(stTree);
        if (option.hasOption("dir")) {
            sdp2js.setOutputDir(option.getOption("dir"));
        }
        sdp2js.generate();
    }
    return 0;
}
