#ifndef _SDP_PARSE_H_
#define _SDP_PARSE_H_

#include <cstdlib>
#include <stack>
#include <set>
#include <tr1/memory>
#include "sdptree.h"
using namespace std;
using namespace std::tr1;

namespace mfw
{
namespace sdp
{

class Grammar
{
public:
    Grammar() {}
    explicit Grammar(const string &s) : m_str(s) {}
    virtual ~Grammar() {}
    const string &str() const
    {
        return m_str;
    }
    template <typename T> T *as()
    {
        return dynamic_cast<T *>(this);
    }

protected:
    string m_str;
};

class StringTok : public Grammar
{
public:
    explicit StringTok(const string &s) : Grammar(s) {}
};

class NumberTok : public Grammar
{
public:
    enum NumberType {
        NumberType_Integer,
        NumberType_Float,
    };

    NumberTok(NumberType type, const string &s) : Grammar(s), m_type(type) {}
    bool isInteger() const
    {
        return m_type == NumberType_Integer;
    }
    bool isFloat() const
    {
        return m_type == NumberType_Float;
    }
    bool isSigned() const
    {
        return m_str[0] == '-';
    }
    int64_t toSignedInt() const
    {
        return (int64_t)toUnsignedInt();
    }
    uint64_t toUnsignedInt() const
    {
        char *end = (char *)m_str.c_str() + m_str.size();
        return strtoull(m_str.c_str(), &end, 0);
    }

private:
    NumberType m_type;
};

class SyntaxIntValue : public Grammar
{
public:
    SyntaxIntValue(int val) : m_val(val) {}
    int val() const
    {
        return m_val;
    }

private:
    int m_val;
};

class SyntaxSdpType : public Grammar
{
public:
    SyntaxSdpType() {}
    explicit SyntaxSdpType(SdpTypeId id) : m_type(id) {}
    SyntaxSdpType(SdpTypeId id, const string &name, const string &fullname) : m_type(id, name, fullname) {}

    SdpType &type()
    {
        return m_type;
    }
private:
    SdpType m_type;
};

class SyntaxAssignValue : public Grammar
{
public:
    SyntaxAssignValue() {}
    SyntaxAssignValue(SdpAssignValue::ValueType type, const string &value) : m_value(type, value) {}
    SyntaxAssignValue(SdpAssignValue::ValueType type, bool b) : m_value(type, b) {}

    SdpAssignValue &value()
    {
        return m_value;
    }
private:
    SdpAssignValue m_value;
};

typedef tr1::shared_ptr<Grammar> GrammarPtr;
typedef tr1::shared_ptr<StringTok> StringTokPtr;
typedef tr1::shared_ptr<NumberTok> NumberTokPtr;
typedef tr1::shared_ptr<SyntaxIntValue> SyntaxIntValuePtr;
typedef tr1::shared_ptr<SyntaxSdpType> SyntaxSdpTypePtr;
typedef tr1::shared_ptr<SyntaxAssignValue> SyntaxAssignValuePtr;
typedef tr1::shared_ptr<SdpParseTree> SdpParseTreePtr;

class ParseContext
{
public:
    explicit ParseContext(const string &sFileName) : iSavedLineno(0), pCurNamespace(NULL), pCurStruct(NULL), pCurEnum(NULL), pCurInterface(NULL), pCurOperation(NULL)
    {
        stTree.sFileName = sFileName;
    }

    SdpParseTree	stTree;
    int				iSavedLineno;
    SdpNamespace	*pCurNamespace;
    SdpStruct		*pCurStruct;
    SdpEnum			*pCurEnum;
    SdpInterface	*pCurInterface;
    SdpInterface::Operation *pCurOperation;
};

typedef tr1::shared_ptr<ParseContext> ParseContextPtr;

enum ClassType {
    ClassType_Enum,
    ClassType_Struct,
    ClassType_Interface,
};

class SdpParser
{
public:
    void error(const string &msg);
    void cleanup();
    ParseContextPtr getCurContext()
    {
        return m_contextStack.top();
    }

    string getCurrentPath();
    int pushFile(const string &sFileName);
    void popFile();
    bool hasFile(const string &sFileName);

    void addClassName(ClassType type, const string &sNamespace, const string &sName)
    {
        m_mNameToClassType[sNamespace + "::" + sName] = type;
    }
    bool hasClassName(const string &sNamespace, const string &sName)
    {
        map<string, ClassType>::const_iterator it;
        if (sName.find("::") != string::npos) {
            it = m_mNameToClassType.find(sName);
        } else {
            it = m_mNameToClassType.find(sNamespace + "::" + sName);
        }
        return it != m_mNameToClassType.end();
    }
    bool hasClassName(ClassType type, const string &sNamespace, const string &sName)
    {
        map<string, ClassType>::const_iterator it;
        if (sName.find("::") != string::npos) {
            it = m_mNameToClassType.find(sName);
        } else {
            it = m_mNameToClassType.find(sNamespace + "::" + sName);
        }
        return it != m_mNameToClassType.end() && it->second == type;
    }

private:
    stack<ParseContextPtr>	m_contextStack;
    set<string>				m_setParseFile;
    map<string, ClassType>	m_mNameToClassType;
};

extern SdpParser g_parser;
}
}

#ifndef YYSTYPE
#define YYSTYPE mfw::sdp::GrammarPtr
#endif

extern int yyparse();

#endif
