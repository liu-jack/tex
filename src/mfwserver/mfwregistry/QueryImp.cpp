#include "QueryImp.h"
#include "RegistryManager.h"
#include "util/util_log.h"

int32_t QueryImp::getEndpoints(const string &sObj, const string &sDivision, vector<string> &vActiveEps, vector<string> &vInactiveEps)
{
    LOG_DEBUG("request: " << sObj << ", div: " << sDivision);

    if (!g_stRegistryManager.isReady()) {
        LOG_DEBUG("data is not ready");
        return -1;
    }
    if (!g_stRegistryManager.getServiceEndpoint(sObj, sDivision, vActiveEps, vInactiveEps)) {
        return -1;
    }
    return 0;
}

int32_t QueryImp::addEndpoint(const string &sObj, const string &sDivision, const string &sEp)
{
    LOG_DEBUG("add endpoint: " << sObj << ", div: " << sDivision << ", ep: " << sEp);

    if (!g_stRegistryManager.isReady()) {
        LOG_DEBUG("data is not ready");
        return -1;
    }

    if (!g_stRegistryManager.addServiceEndpoint(sObj, sDivision, sEp)) {
        return -1;
    }
    return 0;
}

int32_t QueryImp::removeEndpoint(const string &sObj, const string &sDivision, const string &sEp)
{
    LOG_DEBUG("remove endpoint: " << sObj << ", div: " << sDivision << ", ep: " << sEp);

    if (!g_stRegistryManager.isReady()) {
        LOG_DEBUG("data is not ready");
        return -1;
    }

    if (!g_stRegistryManager.removeServiceEndpoint(sObj, sDivision, sEp)) {
        return -1;
    }
    return 0;
}
