%{
#include "parse.h"
#include "lex.yy.hpp"
using namespace mfw::sdp;

#define SYNTAX_ERROR(msg) do { g_parser.error((msg)); YYABORT; } while(0)
#define yyerror SYNTAX_ERROR
%}

%token TOK_IDENTIFIER
%token TOK_VOID
%token TOK_BOOL
%token TOK_CHAR
%token TOK_SHORT
%token TOK_INT
%token TOK_LONG
%token TOK_FLOAT
%token TOK_DOUBLE
%token TOK_STRING
%token TOK_VECTOR
%token TOK_MAP
%token TOK_SIGNED
%token TOK_UNSIGNED
%token TOK_CONST
%token TOK_STRUCT
%token TOK_KEY
%token TOK_ENUM
%token TOK_NAMESPACE
%token TOK_INTERFACE
%token TOK_SCOPE_OPERATOR
%token TOK_OUT
%token TOK_REQUIRED
%token TOK_OPTIONAL
%token TOK_TRUE
%token TOK_FALSE
%token TOK_NUMBER_LITERAL
%token TOK_STRING_LITERAL

%%
definitions: 
  /* empty */
| definitions namespace_def
;

namespace_def:
  TOK_NAMESPACE TOK_IDENTIFIER 
  {
  	SdpNamespace stNamespace;
  	stNamespace.sNamespaceName = $2->str();
  	ParseContextPtr context = g_parser.getCurContext();
  	context->stTree.vNamespace.push_back(stNamespace);
  	context->pCurNamespace = &context->stTree.vNamespace.back();
  }
  '{' content_def '}' ';' {
  	g_parser.getCurContext()->pCurNamespace = NULL;
  }
;

content_def: 
  /* empty */
| content_def struct_def
| content_def key_def
| content_def enum_def
| content_def interface_def
;

struct_def:
  TOK_STRUCT TOK_IDENTIFIER
  {
  	ParseContextPtr context = g_parser.getCurContext();
  	if (g_parser.hasClassName(context->pCurNamespace->sNamespaceName, $2->str()))
  	{
  		SYNTAX_ERROR("struct definition conflict: " + $2->str());
  	}
  	
  	SdpStruct stStruct;
  	stStruct.sStructName = $2->str();
  	context->pCurNamespace->vStruct.push_back(stStruct);
  	context->pCurStruct = &context->pCurNamespace->vStruct.back();
  	g_parser.addClassName(ClassType_Struct, context->pCurNamespace->sNamespaceName, stStruct.sStructName);
  }
  '{' struct_field_list '}' ';' {
  	g_parser.getCurContext()->pCurStruct = NULL;
  }
;

struct_field_list:
  /* empty */
| struct_field_list struct_field_item
;

struct_field_item:
  TOK_NUMBER_LITERAL struct_field_qualifier type_specific TOK_IDENTIFIER default_value_specific ';' {
	NumberTok *pTagTok = $1->as<NumberTok>();
	if (pTagTok->isSigned())
	{
		SYNTAX_ERROR("tag must be positive: " + $1->str());
	}
	uint64_t iTag = pTagTok->toUnsignedInt();
	if (iTag > 65535)
	{
		SYNTAX_ERROR("tag must betwee 0 and 65535: " + $1->str());
	}
	
	ParseContextPtr context = g_parser.getCurContext();
	if (context->pCurStruct->getFieldByTag(iTag) != NULL)
	{
		SYNTAX_ERROR("duplicate struct field tag: " + $1->str());
	}
	if (context->pCurStruct->getField($4->str()) != NULL)
	{
		SYNTAX_ERROR("duplicate struct field name: " + $4->str());
	}
	
	SdpStruct::StructField stStructField;
	stStructField.iTag = iTag;
	stStructField.bRequired = $2->as<SyntaxIntValue>()->val() ? true : false;
	stStructField.stFieldType = $3->as<SyntaxSdpType>()->type();
	stStructField.sFieldName = $4->str();
	if ($5 != NULL)
	{
		stStructField.bHasDefaultValue = true;
		stStructField.stDefaultValue = $5->as<SyntaxAssignValue>()->value();
		if (!stStructField.stDefaultValue.isAssignable(stStructField.stFieldType))
		{
			SYNTAX_ERROR("default value cannot assign with different type: " + stStructField.sFieldName);
		}
	}
	
	vector<SdpStruct::StructField> &vStructField = context->pCurStruct->vStructField;
	unsigned pos = vStructField.size();
	for (unsigned i = 0; i < vStructField.size(); ++i)
	{
		if (stStructField.iTag < vStructField[i].iTag)
		{
			pos = i;
			break;
		}
	}
	vStructField.insert(vStructField.begin() + pos, stStructField);
  }
;

struct_field_qualifier:
  TOK_REQUIRED { $$ = SyntaxIntValuePtr(new SyntaxIntValue(1)); }
| TOK_OPTIONAL { $$ = SyntaxIntValuePtr(new SyntaxIntValue(0)); }
;

type_specific:
  TOK_VOID { $$ = SyntaxSdpTypePtr(new SyntaxSdpType(SdpType_Void)); }
| TOK_BOOL { $$ = SyntaxSdpTypePtr(new SyntaxSdpType(SdpType_Bool)); }
| TOK_CHAR { $$ = SyntaxSdpTypePtr(new SyntaxSdpType(SdpType_Char)); }
| TOK_SIGNED TOK_CHAR { $$ = SyntaxSdpTypePtr(new SyntaxSdpType(SdpType_Int8)); }
| TOK_UNSIGNED TOK_CHAR { $$ = SyntaxSdpTypePtr(new SyntaxSdpType(SdpType_UInt8)); }
| TOK_SHORT { $$ = SyntaxSdpTypePtr(new SyntaxSdpType(SdpType_Int16)); }
| TOK_UNSIGNED TOK_SHORT { $$ = SyntaxSdpTypePtr(new SyntaxSdpType(SdpType_UInt16)); }
| TOK_INT { $$ = SyntaxSdpTypePtr(new SyntaxSdpType(SdpType_Int32)); }
| TOK_UNSIGNED TOK_INT { $$ = SyntaxSdpTypePtr(new SyntaxSdpType(SdpType_UInt32)); }
| TOK_LONG { $$ = SyntaxSdpTypePtr(new SyntaxSdpType(SdpType_Int64)); }
| TOK_UNSIGNED TOK_LONG { $$ = SyntaxSdpTypePtr(new SyntaxSdpType(SdpType_UInt64)); }
| TOK_FLOAT { $$ = SyntaxSdpTypePtr(new SyntaxSdpType(SdpType_Float)); }
| TOK_DOUBLE { $$ = SyntaxSdpTypePtr(new SyntaxSdpType(SdpType_Double)); }
| TOK_STRING { $$ = SyntaxSdpTypePtr(new SyntaxSdpType(SdpType_String)); }
| TOK_VECTOR '<' type_specific '>'  {
	SyntaxSdpTypePtr ptype = SyntaxSdpTypePtr(new SyntaxSdpType(SdpType_Vector)); 
	ptype->type().vInnerType.push_back($3->as<SyntaxSdpType>()->type());
	$$ = ptype;
  }
| TOK_MAP '<' type_specific ',' type_specific '>' {
	SyntaxSdpTypePtr ptype = SyntaxSdpTypePtr(new SyntaxSdpType(SdpType_Map)); 
	ptype->type().vInnerType.push_back($3->as<SyntaxSdpType>()->type());
	ptype->type().vInnerType.push_back($5->as<SyntaxSdpType>()->type());
	$$ = ptype;
  }
| struct_type_specific {
	ParseContextPtr context = g_parser.getCurContext();
	if (!g_parser.hasClassName(ClassType_Struct, context->pCurNamespace->sNamespaceName, $1->str())
		&& !g_parser.hasClassName(ClassType_Enum, context->pCurNamespace->sNamespaceName, $1->str()))
	{
		SYNTAX_ERROR("undefined struct type: " + $1->str());
	}
	SdpTypeId type = g_parser.hasClassName(ClassType_Struct, context->pCurNamespace->sNamespaceName, $1->str()) ? SdpType_Struct : SdpType_Enum;
	const string &sTypeName = $1->str();
	string sTypeFullName = sTypeName;
	if (sTypeName.find("::") == string::npos)
	{
		sTypeFullName = context->pCurNamespace->sNamespaceName + "::" + sTypeName;
	} 
	$$ = SyntaxSdpTypePtr(new SyntaxSdpType(type, sTypeName, sTypeFullName));
  }
;

struct_type_specific:
  TOK_IDENTIFIER
| TOK_IDENTIFIER TOK_SCOPE_OPERATOR struct_type_specific {
	$$ = StringTokPtr(new StringTok($1->str() + "::" + $3->str()));
  }
;

default_value_specific:
  /* empty */ { $$.reset(); }
| '=' TOK_NUMBER_LITERAL { $$ = SyntaxAssignValuePtr(new SyntaxAssignValue($2->as<NumberTok>()->isInteger() ? SdpAssignValue::ValueType_Integer : SdpAssignValue::ValueType_Float, $2->str())); }
| '=' TOK_STRING_LITERAL { $$ = SyntaxAssignValuePtr(new SyntaxAssignValue(SdpAssignValue::ValueType_String, $2->str())); }
| '=' TOK_TRUE { $$ = SyntaxAssignValuePtr(new SyntaxAssignValue(SdpAssignValue::ValueType_Boolean, true)); }
| '=' TOK_FALSE { $$ = SyntaxAssignValuePtr(new SyntaxAssignValue(SdpAssignValue::ValueType_Boolean, false)); }
;

key_def:
  TOK_KEY '[' TOK_IDENTIFIER 
  {
  	ParseContextPtr context = g_parser.getCurContext();
  	context->pCurStruct = context->pCurNamespace->getStruct($3->str());
  	if (context->pCurStruct == NULL)
  	{
  		SYNTAX_ERROR("undefined struct: " + $3->str());
  	}
  }
  key_list ']' ';' {
  	g_parser.getCurContext()->pCurStruct = NULL;
  }
;

key_list:
  key_item
| key_list key_item
;

key_item:
  ',' TOK_IDENTIFIER {
  	ParseContextPtr context = g_parser.getCurContext();
  	SdpStruct &stStruct = *context->pCurStruct;
  	if (stStruct.getField($2->str()) == NULL)
  	{
  		SYNTAX_ERROR("undefined sort key: " + $2->str());
  	}
  	for (unsigned i = 0; i < stStruct.vSortKey.size(); ++i)
  	{
  		if (stStruct.vSortKey[i] == $2->str())
  		{
  			SYNTAX_ERROR("duplicate sort key: " + $2->str());
  		}
  	}
  	stStruct.vSortKey.push_back($2->str());
  }
;

enum_def:
  TOK_ENUM TOK_IDENTIFIER
  {
  	ParseContextPtr context = g_parser.getCurContext();
  	if (g_parser.hasClassName(context->pCurNamespace->sNamespaceName, $2->str()))
  	{
  		SYNTAX_ERROR("enum definition conflict: " + $2->str());
  	}
  	
  	SdpEnum stEnum;
  	stEnum.sEnumName = $2->str();
  	context->pCurNamespace->vEnum.push_back(stEnum);
  	context->pCurEnum = &context->pCurNamespace->vEnum.back();
  	g_parser.addClassName(ClassType_Enum, context->pCurNamespace->sNamespaceName, stEnum.sEnumName);
  }
  '{' enum_list '}' ';' {
  	g_parser.getCurContext()->pCurEnum = NULL;
  }
;

enum_list:
  enum_item
| enum_list ',' enum_item
;

enum_item:
  /* empty */
| TOK_IDENTIFIER default_value_specific {
	ParseContextPtr context = g_parser.getCurContext();
	if (context->pCurEnum->getEnumItem($1->str()) != NULL)
	{
		SYNTAX_ERROR("duplicate enum item: " + $1->str());
	}
	
  	SdpEnum::EnumItem stEnumItem;
  	stEnumItem.sName = $1->str();
  	if ($2 != NULL)
  	{
  		stEnumItem.bHasAssignValue = true;
  		stEnumItem.stAssignValue = $2->as<SyntaxAssignValue>()->value();
  		if (!stEnumItem.stAssignValue.isAssignable(SdpType_Int32))
		{
			SYNTAX_ERROR("default value cannot assign with different type: " + stEnumItem.sName);
		}
  	}
	context->pCurEnum->vEnumItem.push_back(stEnumItem);
  }
;

interface_def:
  TOK_INTERFACE TOK_IDENTIFIER 
  {
  	ParseContextPtr context = g_parser.getCurContext();
  	if (g_parser.hasClassName(context->pCurNamespace->sNamespaceName, $2->str()))
  	{
  		SYNTAX_ERROR("interface definition conflict: " + $2->str());
  	}

  	SdpInterface stInterface;
  	stInterface.sInterfaceName = $2->str();
  	context->pCurNamespace->vInterface.push_back(stInterface);
  	context->pCurInterface = &context->pCurNamespace->vInterface.back();
  	g_parser.addClassName(ClassType_Interface, context->pCurNamespace->sNamespaceName, stInterface.sInterfaceName);
  }
  '{' operation_list '}' ';' {
  	g_parser.getCurContext()->pCurInterface = NULL;
  }
;

operation_list:
  /* empty */
| operation_list operation_item
;

operation_item:
  type_specific TOK_IDENTIFIER
  {
  	ParseContextPtr context = g_parser.getCurContext();
  	if (context->pCurInterface->getOperation($2->str()) != NULL)
  	{
  		SYNTAX_ERROR("duplicate operation: " + $2->str());
  	}
  	SdpInterface::Operation stOperation;
  	stOperation.stRetType = $1->as<SyntaxSdpType>()->type();
  	stOperation.sOperationName = $2->str();
  	context->pCurInterface->vOperation.push_back(stOperation);
  	context->pCurOperation = &context->pCurInterface->vOperation.back();
  }
  '(' paramater_list ')' ';' {
  	g_parser.getCurContext()->pCurOperation = NULL;
  }
;

paramater_list:
  /* empty */
| paramater_item
| paramater_list_leading paramater_item
;

paramater_list_leading:
  paramater_item ','
| paramater_list_leading paramater_item ','
;

paramater_item:
  paramater_out_specific type_specific TOK_IDENTIFIER {
  	ParseContextPtr context = g_parser.getCurContext();
  	if (context->pCurOperation->getParamater($3->str()) != NULL)
  	{
  		SYNTAX_ERROR("duplicate paramater: " + $3->str());
  	}
  	SdpInterface::Paramater stParamater;
  	stParamater.bIsOut = $1->as<SyntaxIntValue>()->val() ? true : false;
  	stParamater.stParamType = $2->as<SyntaxSdpType>()->type();
  	stParamater.sParamName = $3->str();
  	context->pCurOperation->vParam.push_back(stParamater);
  }
;

paramater_out_specific:
  /* empty */ { $$ = SyntaxIntValuePtr(new SyntaxIntValue(0)); }
| TOK_OUT { $$ = SyntaxIntValuePtr(new SyntaxIntValue(1)); }
;

%%
