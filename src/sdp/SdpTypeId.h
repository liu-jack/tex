#ifndef _SDP_TYPE_ID_H_
#define _SDP_TYPE_ID_H_

namespace mfw
{
namespace sdp
{

enum SdpTypeId
{
	SdpType_Void, // 0
	SdpType_Bool, // 1
	SdpType_Char, // 2
	SdpType_Int8, // 3
	SdpType_UInt8, // 4
	SdpType_Int16, // 5
	SdpType_UInt16, // 6
	SdpType_Int32, // 7
	SdpType_UInt32, // 8
	SdpType_Int64, // 9
	SdpType_UInt64, // 10
	SdpType_Float, // 11
	SdpType_Double, // 12
	SdpType_String, // 13
	SdpType_Vector, // 14
	SdpType_Map, // 15
	SdpType_Enum, // 16
	SdpType_Struct, // 17
};

}
}

#endif
