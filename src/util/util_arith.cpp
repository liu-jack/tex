#include "util_arith.h"

namespace mfw
{

uint32_t UtilArith::safeAdd(uint32_t a, uint32_t b)
{
	a = a + b;
	if (a < b) return (uint32_t)-1;
	return a;
}

uint32_t UtilArith::safeAdd(uint32_t a, uint32_t b, uint32_t max)
{
	uint32_t val = safeAdd(a, b);
	return val > max ? max : val;
}

uint32_t UtilArith::safeSub(uint32_t a, uint32_t b)
{
	if (b > a) return 0;
	return a - b;
}

uint32_t UtilArith::safeSub(uint32_t a, uint32_t b, uint32_t min)
{
	uint32_t val = safeSub(a, b);
	return val < min ? min : val;
}

uint64_t UtilArith::safeAdd64(uint64_t a, uint64_t b)
{
	a = a + b;
	if (a < b) return (uint64_t)-1;
	return a;
}

uint64_t UtilArith::safeAdd64(uint64_t a, uint64_t b, uint64_t max)
{
	uint64_t val = safeAdd(a, b);
	return val > max ? max : val;
}

uint64_t UtilArith::safeSub64(uint64_t a, uint64_t b)
{
	if (b > a) return 0;
	return a - b;
}

uint64_t UtilArith::safeSub64(uint64_t a, uint64_t b, uint64_t min)
{
	uint64_t val = safeSub(a, b);
	return val < min ? min : val;
}

};
