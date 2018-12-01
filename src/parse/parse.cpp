#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdlib.h>
#include <iostream>
#include "util/util_file.h"
#include "parse.h"
#include "lex.yy.hpp"
using namespace std;
using namespace mfw;
using namespace mfw::sdp;

namespace mfw
{
namespace sdp
{

SdpParser g_parser;

string SdpParser::getCurrentPath()
{
    if (m_contextStack.empty()) {
        return ".";
    }
    return UtilFile::getFileDirname(m_contextStack.top()->stTree.sFileName);
}

int SdpParser::pushFile(const string &sFileName)
{
    string sAbsFile = UtilFile::getAbsolutePath(sFileName, getCurrentPath());
    yyin = fopen(sAbsFile.c_str(), "r");
    if (yyin == NULL) {
        return -1;
    }
    yypush_buffer_state(yy_create_buffer(yyin, /*YY_BUF_SIZE*/ 16 * 1024));

    if(!m_contextStack.empty()) {
        m_contextStack.top()->iSavedLineno = yylineno;
        yylineno = 1;
    }

    ParseContextPtr context = ParseContextPtr(new ParseContext(sAbsFile));
    m_contextStack.push(context);

    m_setParseFile.insert(sAbsFile);
    //cout << "Analyzing file: " << sAbsFile << endl;
    return 0;
}

void SdpParser::popFile()
{
    m_contextStack.pop();
    if (!m_contextStack.empty()) {
        yylineno = m_contextStack.top()->iSavedLineno;
    }

    if (yyin != NULL) {
        fclose(yyin);
        yyin = NULL;
    }
    yypop_buffer_state();
}

bool SdpParser::hasFile(const string &sFileName)
{
    string sAbsFile = UtilFile::getAbsolutePath(sFileName, getCurrentPath());
    return m_setParseFile.find(sAbsFile) != m_setParseFile.end();
}

void SdpParser::error(const string &msg)
{
    if (m_contextStack.empty()) {
        cerr << "error: " << msg << endl;
    } else {
        cerr << m_contextStack.top()->stTree.sFileName << ":" << yylineno << ": " << msg << endl;
    }
}

void SdpParser::cleanup()
{
    while (yyin != NULL) {
        popFile();
    }
    while (!m_contextStack.empty()) {
        m_contextStack.pop();
    }
    m_setParseFile.clear();
    m_mNameToClassType.clear();
}

}
}
