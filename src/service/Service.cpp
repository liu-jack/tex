#include "service/Service.h"
#include "util/util_singleton.h"

namespace mfw
{

CallbackThreadData *CallbackThreadData::getData()
{
    CallbackThreadData *pCbtd = ThreadSingleton<CallbackThreadData>::get();
    return pCbtd;
}

}
