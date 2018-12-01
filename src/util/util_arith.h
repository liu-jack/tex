#ifndef _MFW_UTIL_ARITH_H_
#define _MFW_UTIL_ARITH_H_

#include <stdint.h>

namespace mfw
{
class UtilArith
{
public:
    static uint32_t safeAdd(uint32_t a, uint32_t b);
    static uint32_t safeAdd(uint32_t a, uint32_t b, uint32_t max);

    static uint32_t safeSub(uint32_t a, uint32_t b);
    static uint32_t safeSub(uint32_t a, uint32_t b, uint32_t min);

    static uint64_t safeAdd64(uint64_t a, uint64_t b);
    static uint64_t safeAdd64(uint64_t a, uint64_t b, uint64_t max);

    static uint64_t safeSub64(uint64_t a, uint64_t b);
    static uint64_t safeSub64(uint64_t a, uint64_t b, uint64_t min);
};
};

#endif
