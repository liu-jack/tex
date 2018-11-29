#ifndef _MFW_UTIL_RANDOM_
#define _MFW_UTIL_RANDOM_

#include <stdint.h>

namespace mfw
{

class UtilRandom
{
public:
	static uint32_t random32();
	static uint32_t random32(uint32_t min, uint32_t max);
};

}

#endif
