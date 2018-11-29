%option noyywrap
%option yylineno
%{
#include "parse.h"
#include "syntax.tab.hpp"
using namespace mfw::sdp;

#define LEX_ERROR(msg) do { g_parser.error((msg)); yyterminate(); } while(0)

#include <iostream>
using namespace std;
//#define LOGDEBUG(x) do { cout << yylineno << ":" << x << endl; } while (0)
#define LOGDEBUG(x) do {} while (0)

map<string, int> initTokenType()
{
	map<string, int> m;
	m["void"] = TOK_VOID;
    m["bool"] = TOK_BOOL;
    m["byte"] = TOK_CHAR;
    m["char"] = TOK_CHAR;
	m["short"] = TOK_SHORT;
	m["int"] = TOK_INT;
	m["long"] = TOK_LONG;
	m["float"] = TOK_FLOAT;
	m["double"] = TOK_DOUBLE;
	m["string"] = TOK_STRING;
	m["vector"] = TOK_VECTOR;
	m["map"] = TOK_MAP;
	m["signed"] = TOK_SIGNED;
	m["unsigned"] = TOK_UNSIGNED;
	m["const"] = TOK_CONST;
	m["struct"] = TOK_STRUCT;
	m["key"] = TOK_KEY;
	m["enum"] = TOK_ENUM;
	m["namespace"] = TOK_NAMESPACE;
	m["module"] = TOK_NAMESPACE;
	m["interface"] = TOK_INTERFACE;
	m["out"] = TOK_OUT;
	m["require"] = TOK_REQUIRED;
	m["required"] = TOK_REQUIRED;
	m["optional"] = TOK_OPTIONAL;
	m["true"] = TOK_TRUE;
	m["false"] = TOK_FALSE;
	return m;
}
static map<string, int> g_mTokenType = initTokenType();
%}

identifier		[[:alpha:]_][[:alnum:]_]*
integer_literal	[+-]?((0[0-7]+)|(0[xX][[:xdigit:]]+)|([[:digit:]]+))
float_literal	[+-]?(([[:digit:]]*\.[[:digit:]]+)|([[:digit:]]+\.))

%x INCL
%%

"#include"[ \t]* { BEGIN(INCL); }

<INCL>(\"[^"]*\")|(<[^>]*>) {
	string sText = string(yytext + 1, yytext + strlen(yytext) - 1);
	LOGDEBUG("found include: " << sText);
	if (g_parser.getCurContext()->pCurNamespace != NULL)
	{
		LEX_ERROR("include must outside namespace: " + sText);
	}
	
	BEGIN(INITIAL);
	if (!g_parser.hasFile(sText))
	{
		g_parser.getCurContext()->stTree.vInclude.push_back(sText);
		int ret = g_parser.pushFile(sText);
		if (ret != 0)
		{
			LEX_ERROR("fail to parse file: " + sText);
		}
	}
}

<INCL>\n {
	LEX_ERROR("Invalid include");
}

<INCL><<EOF>> {
	LEX_ERROR("Invalid include");
}

<<EOF>> {
	if (yy_buffer_stack_top > 0)
	{
		g_parser.popFile();
	}
	else
	{
		yyterminate();
	}
}

"//" {
	int c;
	string sText;
	while ((c = yyinput()) != EOF)
	{
		if (c == '\n')
		{
			break;
		}
		sText.push_back((char)c);
	}
	LOGDEBUG("found // comment: " << sText);
}

"/*" {
	int c;
	string sText;
	while ((c = yyinput()) != EOF)
	{
		if (c != '*')
		{
			sText.push_back((char)c);
			continue;
		}
		
		int next = yyinput();
		if (next != '/')
		{
			sText.push_back((char)c);
			unput(next);
			continue;
		}
		break;
	}
	if (c == EOF)
	{
		LEX_ERROR("EOF in comment");
	}
	else
	{
		LOGDEBUG("found /* comment: " << sText);
	}
}

\" {
	int c;
	string sText;
	while ((c = yyinput()) != EOF)
	{
		if (c == '\"')
		{
			break;
		}
		
		sText.push_back((char)c);
		if (c == '\\')
		{
			c = yyinput();
			if (c != EOF)
			{
				sText.push_back((char)c);
			}
		}
	}
	
	if (c == EOF)
	{
		LEX_ERROR("EOF in string");
	}
	else
	{
		LOGDEBUG("found string: " << c_unescape(sText));
		yylval = StringTokPtr(new StringTok(sText));
		return TOK_STRING_LITERAL;
	}
}

{identifier} {
	
	map<string, int>::const_iterator it = g_mTokenType.find(yytext);
	if (it != g_mTokenType.end())
	{
		LOGDEBUG("found keyword: " << yytext);
		return it->second;
	}

	LOGDEBUG("found identifier: " << yytext);
	yylval = StringTokPtr(new StringTok(yytext));
	return TOK_IDENTIFIER;
}

{integer_literal} {
	LOGDEBUG("found integer: " << yytext);
	yylval = NumberTokPtr(new NumberTok(NumberTok::NumberType_Integer, yytext));
	return TOK_NUMBER_LITERAL;
}

{float_literal} {
	LOGDEBUG("found float: " << yytext);
	yylval = NumberTokPtr(new NumberTok(NumberTok::NumberType_Float, yytext));
	return TOK_NUMBER_LITERAL;
}

:: {
	LOGDEBUG("found scope operator: " << yytext);
	return TOK_SCOPE_OPERATOR;
}

[[:space:]] { }

. {
	LOGDEBUG("found char: " << yytext[0]);
	return yytext[0];
}
