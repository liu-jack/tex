#ifndef _LOG_IMP_H
#define _LOG_IMP_H

#include "log/Log.h"
using namespace mfw;

class LogImp : public Log
{
public:
    LogImp() {}
    virtual ~LogImp() {}
    virtual void initialize() {}
    virtual void destroy() {}

    virtual int32_t logRemote(const mfw::LogBaseInfo &info, const map<string, vector<mfw::LogDataItem> > &mFileData);
};

#endif

