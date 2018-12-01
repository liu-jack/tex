#include "service/RemoteLog.h"
#include "service/Application.h"
#include "log/Log.h"
#include "util/util_log.h"

namespace mfw
{

static LogPrx g_remoteLogPrx;
static LogPrx g_remoteLogGlobalPrx;

static void processRemoteLog(list<MfwLogData> &listLogData)
{
    if (!g_remoteLogPrx && !g_remoteLogGlobalPrx) {
        return;
    }

    map<string, vector<mfw::LogDataItem> > mFileData, mGlolbalFileData;
    for (list<MfwLogData>::iterator first = listLogData.begin(), last = listLogData.end(); first != last; ++first) {
        MfwLogData &data = *first;
        MfwLogType iLogType = CLogManager::getInstance()->getLogType(data.log);
        if (iLogType == MfwLogType_Day || iLogType == MfwLogType_GlobalDay || iLogType == MfwLogType_Hour || iLogType == MfwLogType_GlobalHour) {
            string sLogName = CLogManager::getInstance()->getLogFileName(data.log) + "_";
            if (iLogType == MfwLogType_Day || iLogType == MfwLogType_GlobalDay) {
                sLogName += UtilTime::formatTime(data.iTime, "%Y%m%d");
            } else {
                sLogName += UtilTime::formatTime(data.iTime, "%Y%m%d_%H");
            }

            mfw::LogDataItem stLogDataItem;
            stLogDataItem.iTime = data.iTime;
            stLogDataItem.sData = data.sData;

            if (iLogType == MfwLogType_Day || iLogType == MfwLogType_Hour) {
                vector<mfw::LogDataItem> &vLogDataItem = mFileData[sLogName];
                vLogDataItem.push_back(stLogDataItem);
            } else {
                vector<mfw::LogDataItem> &vLogDataItem = mGlolbalFileData[sLogName];
                vLogDataItem.push_back(stLogDataItem);
            }
        }
    }
    if (mFileData.empty() && mGlolbalFileData.empty()) {
        return;
    }

    mfw::LogBaseInfo info;
    info.sAppName = ServerConfig::Application;
    info.sServerName  = ServerConfig::ServerName;
    info.sDivision = ClientConfig::SetDivision;

    if (!mFileData.empty() && g_remoteLogPrx) {
        try {
            g_remoteLogPrx->logRemote(info, mFileData);
        } catch (...) {
        }
    }

    if (!mGlolbalFileData.empty() && g_remoteLogGlobalPrx) {
        info.sDivision.clear();

        try {
            g_remoteLogGlobalPrx->logRemote(info, mGlolbalFileData);
        } catch (...) {
        }
    }
}

void MfwRemoteLog::initRemoteLog(const string &sLogObj, const string &sGlobalLogObj)
{
    if (!sLogObj.empty()) {
        g_remoteLogPrx = g_app->getConnector()->stringToProxy<LogPrx>(sLogObj);
    }
    if (!sGlobalLogObj.empty()) {
        g_remoteLogGlobalPrx = g_app->getConnector()->stringToProxy<LogPrx>(sGlobalLogObj);
    }
    if (!sLogObj.empty() || !sGlobalLogObj.empty()) {
        CLogManager::getInstance()->setRemoteCallback(processRemoteLog);
    }
}

}
