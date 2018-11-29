#ifndef _MFW_SDP_PARSE_SDPTREE_H_
#define _MFW_SDP_PARSE_SDPTREE_H_

#include <stdint.h>
#include <string>
#include <vector>
#include <map>
#include "sdp/SdpTypeId.h"
using namespace std;

namespace mfw
{
namespace sdp
{

class SdpType
{
public:
	SdpType() : iTypeId(SdpType_Void) {}
	explicit SdpType(SdpTypeId id) : iTypeId(id) {}
	SdpType(SdpTypeId id, const string &name, const string &fullname) : iTypeId(id), sName(name), sFullName(fullname) {}

	SdpTypeId iTypeId;
	string sName;
	string sFullName;
	vector<SdpType> vInnerType;
};

class SdpAssignValue
{
public:
	enum ValueType
	{
		ValueType_Boolean,
		ValueType_Integer,
		ValueType_Float,
		ValueType_String,
	};

	SdpAssignValue() : iType(ValueType_Boolean), bBoolean(false) {}
	SdpAssignValue(ValueType type, const string &value) : iType(type), sValue(value), bBoolean(false) {}
	SdpAssignValue(ValueType type, bool b) : iType(type), bBoolean(b) {}

	bool isAssignable(const SdpType &type)
	{
		return isAssignable(type.iTypeId);
	}
	bool isAssignable(SdpTypeId id)
	{
		switch (id)
		{
		case SdpType_Void: return false;
		case SdpType_Bool: return iType == ValueType_Boolean;
		case SdpType_Char:
		case SdpType_Int8:
		case SdpType_UInt8:
		case SdpType_Int16:
		case SdpType_UInt16:
		case SdpType_Int32:
		case SdpType_UInt32:
		case SdpType_Int64:
		case SdpType_UInt64: return iType == ValueType_Integer;
		case SdpType_Float:
		case SdpType_Double: return iType == ValueType_Integer || iType == ValueType_Float;
		case SdpType_String: return iType == ValueType_String;
		case SdpType_Vector:
		case SdpType_Map:
		case SdpType_Enum: return iType == ValueType_Integer;
		case SdpType_Struct: return false;
		}
		return false;
	}

	ValueType	iType;
	string		sValue;
	bool		bBoolean;
};

class SdpEnum
{
public:
	struct EnumItem
	{
		string		sName;
		bool		bHasAssignValue;
		SdpAssignValue stAssignValue;

		EnumItem() : bHasAssignValue(false) {}
	};

	EnumItem *getEnumItem(const string &sName)
	{
		for (unsigned i = 0; i < vEnumItem.size(); ++i)
		{
			if (vEnumItem[i].sName == sName)
			{
				return &vEnumItem[i];
			}
		}
		return NULL;
	}

	string				sEnumName;
	vector<EnumItem>	vEnumItem;
};

class SdpStruct
{
public:
	struct StructField
	{
		uint32_t	iTag;
		bool		bRequired;
		SdpType		stFieldType;
		string		sFieldName;
		bool		bHasDefaultValue;
		SdpAssignValue stDefaultValue;

		StructField() : iTag(0), bRequired(false), bHasDefaultValue(false) {}
	};

	StructField *getField(const string &sFieldName)
	{
		for (unsigned i = 0; i < vStructField.size(); ++i)
		{
			if (vStructField[i].sFieldName == sFieldName)
			{
				return &vStructField[i];
			}
		}
		return NULL;
	}
	StructField *getFieldByTag(uint32_t iTag)
	{
		for (unsigned i = 0; i < vStructField.size(); ++i)
		{
			if (vStructField[i].iTag == iTag)
			{
				return &vStructField[i];
			}
		}
		return NULL;
	}

	string				sStructName;
	vector<StructField>	vStructField;
	vector<string>		vSortKey;
};

class SdpInterface
{
public:
	struct Paramater
	{
		bool		bIsOut;
		SdpType		stParamType;
		string		sParamName;

		Paramater() : bIsOut(false) {}
	};

	struct Operation
	{
		SdpType		stRetType;
		string		sOperationName;
		vector<Paramater> vParam;

		Paramater *getParamater(const string &sParamName)
		{
			for (unsigned i = 0; i < vParam.size(); ++i)
			{
				if (vParam[i].sParamName == sParamName)
				{
					return &vParam[i];
				}
			}
			return NULL;
		}
	};

	Operation *getOperation(const string &sOperationName)
	{
		for (unsigned i = 0; i < vOperation.size(); ++i)
		{
			if (vOperation[i].sOperationName == sOperationName)
			{
				return &vOperation[i];
			}
		}
		return NULL;
	}

	string					sInterfaceName;
	vector<Operation>		vOperation;
};

class SdpNamespace
{
public:
	string					sNamespaceName;
	vector<SdpEnum>			vEnum;
	vector<SdpStruct>		vStruct;
	vector<SdpInterface>	vInterface;

	SdpEnum *getEnum(const string &sEnumName)
	{
		for (unsigned i = 0; i < vEnum.size(); ++i)
		{
			if (vEnum[i].sEnumName == sEnumName)
			{
				return &vEnum[i];
			}
		}
		return NULL;
	}

	SdpStruct *getStruct(const string &sStructName)
	{
		for (unsigned i = 0; i < vStruct.size(); ++i)
		{
			if (vStruct[i].sStructName == sStructName)
			{
				return &vStruct[i];
			}
		}
		return NULL;
	}

	SdpInterface *getInterface(const string &sInterfaceName)
	{
		for (unsigned i = 0; i < vInterface.size(); ++i)
		{
			if (vInterface[i].sInterfaceName == sInterfaceName)
			{
				return &vInterface[i];
			}
		}
		return NULL;
	}
};

class SdpParseTree
{
public:
	string					sFileName;
	vector<string>			vInclude;
	vector<SdpNamespace>	vNamespace;

	bool hasInterface() const
	{
		for (unsigned i = 0; i < vNamespace.size(); ++i)
		{
			if (!vNamespace[i].vInterface.empty())
			{
				return true;
			}
		}
		return false;
	}
};

static inline void swap(SdpParseTree &a, SdpParseTree &b)
{
	swap(a.sFileName, b.sFileName);
	swap(a.vInclude, b.vInclude);
	swap(a.vNamespace, b.vNamespace);
}

int parsesdp(const string &sFileName, SdpParseTree &stTree);
string dumpsdp(const SdpParseTree &stTree);

}
}

#endif
