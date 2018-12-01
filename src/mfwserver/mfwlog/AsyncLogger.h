#ifndef __ASYNC_LOGGER_H
#define __ASYNC_LOGGER_H

#include "log/Log.h"
#include "util/util_queue.h"
using namespace mfw;

struct LogServerConfig {
    string	sLogPath;
};
extern LogServerConfig g_stLogServerConfig;

void asyncLoggerThread(CThread &thread);
void addToAsyncLogger(const mfw::LogBaseInfo &info, const map<string, vector<mfw::LogDataItem> > &mFileData, const string &sIp);

#endif
