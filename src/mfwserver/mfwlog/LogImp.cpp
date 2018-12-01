#include "LogImp.h"
#include "AsyncLogger.h"

int32_t LogImp::logRemote(const mfw::LogBaseInfo &info, const map<string, vector<mfw::LogDataItem> > &mFileData)
{
    try {
        addToAsyncLogger(info, mFileData, getCurrent()->getIp());
    } catch (...) {
    }
    return 0;
}
