#include "util_log.h"
#include "util_string.h"
#include "util_time.h"
#include "util_file.h"
#include "util_thread.h"
#include "util_encode.h"
#include <stdio.h>
#include <cstring>
#include <iostream>
#include <map>
#include <list>
using namespace std;

namespace mfw
{

const char *briefLogFileName(const char *name)
{
    if (name[0] != '/') {
        return name;
    }
    const char *p = strrchr(name, '/');
    if (p != NULL) {
        return p + 1;
    }
    return name;
}

class CLog
{
public:
    CLog(const string &sPath, const string &sFileNamePrefix, const string &sFileName, MfwLogType type):
        m_sPath(sPath), m_sFileName(sFileName), m_type(type), m_iFileSize(0), m_fp(NULL)
    {
        if (m_sPath.empty()) {
            m_sPath = ".";
        }
        m_sLogFilePahtPrefix = m_sPath + "/" + sFileNamePrefix;
        if (!m_sFileName.empty()) {
            m_sLogFilePahtPrefix += "_" + m_sFileName;
        }
    }

    virtual ~CLog() {}

    MfwLogType getLogType() const
    {
        return m_type;
    }
    const string &getFileName() const
    {
        return m_sFileName;
    }

    void writeLog(const MfwLogData &data)
    {
        if (prepareLogFile(data)) {
            uint64_t n = 0;
            string sTime = UtilTime::formatTime(data.iTime, "%Y-%m-%d %H:%M:%S|");
            n += fwrite(sTime.c_str(), 1, sTime.size(), m_fp);
            n += fwrite(data.sData.c_str(), 1, data.sData.size(), m_fp);
            m_iFileSize += n;
            fflush(m_fp);
        }
    }

    virtual void closeExpiredFile() {}

protected:
    const string &getLogFilePathPrefix() const
    {
        return m_sLogFilePahtPrefix;
    }
    virtual bool prepareLogFile(const MfwLogData &data) = 0;

protected:
    string m_sPath;
    string m_sFileName;
    string m_sLogFilePahtPrefix;
    MfwLogType m_type;
    uint64_t m_iFileSize;
    FILE *m_fp;
};

class CRollLog : public CLog
{
public:
    CRollLog(const string &sPath, const string &sFileNamePrefix, const string &sFileName) :
        CLog(sPath, sFileNamePrefix, sFileName, MfwLogType_Roll),
        m_iRollLogMaxFile(10), m_iRollLogMaxSize(100 * 1024 * 1024)
    {
    }

    virtual ~CRollLog()
    {
        if (m_fp != NULL) {
            fclose(m_fp);
            m_fp = NULL;
        }
    }

    void setRollLogInfo(uint32_t iMaxFile, uint64_t iMaxSize)
    {
        m_iRollLogMaxFile = std::max(static_cast<uint32_t>(1), iMaxFile);
        m_iRollLogMaxSize = std::max(static_cast<uint64_t>(128), iMaxSize);
    }

protected:
    string getRollLogFileName(uint32_t iIndex)
    {
        if (iIndex == 0) {
            return getLogFilePathPrefix() + ".log";
        }
        return getLogFilePathPrefix() + UtilString::tostr(iIndex) + ".log";
    }

    virtual bool prepareLogFile(const MfwLogData &data)
    {
        if (m_fp == NULL) {
            // try to append to existing log
            string sCurrentRollFile = getRollLogFileName(0);
            m_fp = fopen(sCurrentRollFile.c_str(), "a+");
            if (m_fp == NULL) {
                return false;
            }
            fseeko(m_fp, 0, SEEK_END);
            m_iFileSize = ftello(m_fp);
        }

        // append
        if (m_iFileSize < m_iRollLogMaxSize) {
            return true;
        }

        // close
        fclose(m_fp);
        m_fp = NULL;
        m_iFileSize = 0;

        // roll log file
        UtilFile::removeFile(getRollLogFileName(m_iRollLogMaxFile - 1));
        for (int i = m_iRollLogMaxFile - 2; i >= 0; --i) {
            UtilFile::moveFile(getRollLogFileName(i), getRollLogFileName(i + 1));
        }

        return prepareLogFile(data);
    }

private:
    uint32_t m_iRollLogMaxFile;
    uint64_t m_iRollLogMaxSize;
};

struct LogFileInfo {
    FILE *fp;
    uint32_t iAccessTime;
};

class CDayLog : public CLog
{
public:
    CDayLog(const string &sPath, const string &sFileNamePrefix, const string &sFileName, MfwLogType type) :
        CLog(sPath, sFileNamePrefix, sFileName, type)
    {
    }

    virtual ~CDayLog()
    {
        for (map<uint32_t, LogFileInfo>::iterator first = m_mDateFile.begin(), last = m_mDateFile.end(); first != last; ++first) {
            FILE *fp = first->second.fp;
            if (fp != NULL) {
                fclose(fp);
            }
        }
        m_fp = NULL;
        m_mDateFile.clear();
    }

    virtual void closeExpiredFile()
    {
        if (m_mDateFile.empty()) {
            return;
        }

        uint32_t iNow = UtilTime::getNow();
        uint32_t iExpiredTime = iNow - 60;
        for (map<uint32_t, LogFileInfo>::iterator it = m_mDateFile.begin(); it != m_mDateFile.end(); ) {
            LogFileInfo &info = it->second;
            if (info.iAccessTime < iExpiredTime) {
                if (info.fp != NULL) {
                    fclose(info.fp);
                }
                m_mDateFile.erase(it++);
            } else {
                ++it;
            }
        }
    }

protected:
    bool prepareLogFile(const MfwLogData &data)
    {
        uint32_t iDate = UtilTime::getDate(data.iTime);
        map<uint32_t, LogFileInfo>::iterator it = m_mDateFile.find(iDate);
        if (it != m_mDateFile.end()) {
            LogFileInfo &info = it->second;
            info.iAccessTime = UtilTime::getNow();
            m_fp = info.fp;
            return true;
        }

        string sCurrentDayFile = getLogFilePathPrefix() + "_" + UtilString::tostr(iDate) + ".log";
        m_fp = fopen(sCurrentDayFile.c_str(), "a+");
        if (m_fp == NULL) {
            return false;
        }
        LogFileInfo &info = m_mDateFile[iDate];
        info.fp = m_fp;
        info.iAccessTime = UtilTime::getNow();
        return true;
    }

protected:
    map<uint32_t, LogFileInfo> m_mDateFile;
};

class CHourLog : public CDayLog
{
public:
    CHourLog(const string &sPath, const string &sFileNamePrefix, const string &sFileName, MfwLogType type) :
        CDayLog(sPath, sFileNamePrefix, sFileName, type)
    {
    }

protected:
    bool prepareLogFile(const MfwLogData &data)
    {
        uint32_t iDateHour = UtilTime::getDate(data.iTime) * 100 + UtilTime::getHour(data.iTime);
        map<uint32_t, LogFileInfo>::iterator it = m_mDateFile.find(iDateHour);
        if (it != m_mDateFile.end()) {
            LogFileInfo &info = it->second;
            info.iAccessTime = UtilTime::getNow();
            m_fp = info.fp;
            return true;
        }

        char buf[32];
        snprintf(buf, sizeof(buf), "%d_%d", iDateHour / 100, iDateHour % 100);
        string sCurrentDayFile = getLogFilePathPrefix() + "_" + buf + ".log";
        m_fp = fopen(sCurrentDayFile.c_str(), "a+");
        if (m_fp == NULL) {
            return false;
        }
        LogFileInfo &info = m_mDateFile[iDateHour];
        info.fp = m_fp;
        info.iAccessTime = UtilTime::getNow();
        return true;
    }
};

static CRollLog g_dummyStdoutLog(".", "", "");

CLogManager::CLogManager()
    : m_mfwLogLevel(MfwLogLevel_Debug)
    , m_mfwFrameworkLogLevel(MfwLogLevel_Info)
    , m_mfwLogThread("logger")
    , m_mfwLogEnabled(false)
{}

void CLogManager::cleanupMfwLog()
{
    CLockGuard<CMutex> lock(m_mfwLogMutex);
    for (map<string, CLog *>::iterator first = m_mAllMfwLog.begin(), last = m_mAllMfwLog.end(); first != last; ++first) {
        CLog *log = first->second;
        delete log;
    }
    m_mAllMfwLog.clear();
    m_listLogData.clear();
}

void CLogManager::loggerThreadEntry(CThread &thread)
{
    uint32_t iLastCheckTime = UtilTime::getNow();
    while (true) {
        CLockGuard<CMutex> lock(m_mfwLogMutex);

        uint32_t iNow = UtilTime::getNow();
        if (iLastCheckTime + 10 < iNow) {
            for (map<string, CLog *>::iterator first = m_mAllMfwLog.begin(), last = m_mAllMfwLog.end(); first != last; ++first) {
                CLog *log = first->second;
                log->closeExpiredFile();
            }
            iLastCheckTime = iNow;
        }

        list<MfwLogData> listLogData;
        swap(m_listLogData, listLogData);
        lock.unlock();

        if (listLogData.empty()) {
            if (thread.isTerminate()) {
                break;
            }
            thread.timedwait(1000);
            continue;
        }

        for (list<MfwLogData>::iterator first = listLogData.begin(), last = listLogData.end(); first != last; ++first) {
            MfwLogData &data = *first;
            data.log->writeLog(data);
        }

        tr1::function<void(list<MfwLogData> &)> remoteCallback;
        lock.lock();
        remoteCallback = m_remoteCallback;
        lock.unlock();

        if (remoteCallback) {
            remoteCallback(listLogData);
        }
    }
    cleanupMfwLog();
}

CLog *CLogManager::getInitLogByType(const string &sFileName, MfwLogType type)
{
    CLockGuard<CMutex> lock(m_mfwLogMutex);
    if (!m_mfwLogEnabled) {
        return NULL;
    }
    if (m_mfwLogPrefix.empty()) {
        return &g_dummyStdoutLog;
    }
    if (sFileName.find('/') != string::npos) {
        return NULL;
    }

    map<string, CLog *>::iterator it = m_mAllMfwLog.find(sFileName);
    if (it != m_mAllMfwLog.end()) {
        CLog *log = it->second;
        if (log->getLogType() != type) {
            return NULL;
        }
        return log;
    }

    CLog *log = NULL;
    if (type == MfwLogType_Roll) {
        log = new CRollLog(m_mfwLogPath, m_mfwLogPrefix, sFileName);
    } else if (type == MfwLogType_Day || type == MfwLogType_GlobalDay || type == MfwLogType_LocalGlobalDay || type == MfwLogType_LocalDay) {
        log = new CDayLog(m_mfwLogPath, m_mfwLogPrefix, sFileName, type);
    } else if (type == MfwLogType_Hour || type == MfwLogType_GlobalHour || type == MfwLogType_LocalHour || MfwLogType_LocalGlobalHour) {
        log = new CHourLog(m_mfwLogPath, m_mfwLogPrefix, sFileName, type);
    } else {
        return NULL;
    }
    m_mAllMfwLog[sFileName] = log;
    return log;
}

void CLogManager::initLog(const string &sLogPath, const string &sLogPrefix)
{
    CLockGuard<CMutex> lock(m_mfwLogMutex);
    if (!m_mfwLogEnabled) {
        m_mfwLogPath = UtilFile::getAbsolutePath(sLogPath);
        UtilFile::makeDirectoryRecursive(m_mfwLogPath);

        m_mfwLogPrefix = sLogPrefix;
        m_mfwLogEnabled = true;
        m_mfwLogThread.setRoutine(tr1::bind(&CLogManager::loggerThreadEntry, this, tr1::placeholders::_1));
        m_mfwLogThread.start();
    }
}

void CLogManager::finiLog()
{
    CLockGuard<CMutex> lock(m_mfwLogMutex);
    if (m_mfwLogEnabled) {
        m_mfwLogEnabled = false;
        m_mfwLogThread.terminate();
        lock.unlock();

        m_mfwLogThread.join();
    }
}

void CLogManager::setRemoteCallback(tr1::function<void(list<MfwLogData> &)> cb)
{
    CLockGuard<CMutex> lock(m_mfwLogMutex);
    m_remoteCallback = cb;
}

void CLogManager::setRollLogInfo(CLog *log, uint32_t iMaxFile, uint64_t iMaxSize)
{
    if (log != NULL) {
        CLockGuard<CMutex> lock(m_mfwLogMutex);
        if (log->getLogType() == MfwLogType_Roll) {
            CRollLog *rolllog = static_cast<CRollLog *>(log);
            rolllog->setRollLogInfo(iMaxFile, iMaxSize);
        }
    }
}

void CLogManager::setLogLevel(MfwLogLevel level, MfwLogLevel frameworkLevel)
{
    CLockGuard<CMutex> lock(m_mfwLogMutex);
    m_mfwLogLevel = level;
    m_mfwFrameworkLogLevel = frameworkLevel;
}

static MfwLogLevel parseLogLevel(const string &sLevel)
{
    MfwLogLevel level = MfwLogLevel_Debug;
    string s = UtilString::toupper(sLevel);
    if (s == "DEBUG") {
        level = MfwLogLevel_Debug;
    } else if (s == "INFO") {
        level = MfwLogLevel_Info;
    } else if (s == "ERROR") {
        level = MfwLogLevel_Error;
    } else if (s == "NONE") {
        level = MfwLogLevel_None;
    }
    return level;
}

void CLogManager::setLogLevel(const string &sLevel, const string &sFrameworkLevel)
{
    setLogLevel(parseLogLevel(sLevel), parseLogLevel(sFrameworkLevel));
}

CLog *CLogManager::getRollLog(const string &sFileName)
{
    return getInitLogByType(sFileName, MfwLogType_Roll);
}

CLog *CLogManager::getDayLog(const string &sFileName, bool bGlobal, bool bLocal)
{
    MfwLogType type = !bLocal ? (bGlobal ? MfwLogType_GlobalDay : MfwLogType_Day) : (bGlobal ? MfwLogType_LocalGlobalDay : MfwLogType_LocalDay);
    return getInitLogByType(sFileName, type);
}

CLog *CLogManager::getHourLog(const string &sFileName, bool bGlobal, bool bLocal)
{
    MfwLogType type = !bLocal ? (bGlobal ? MfwLogType_GlobalHour : MfwLogType_Hour) : (bGlobal ? MfwLogType_LocalGlobalHour : MfwLogType_LocalHour);
    return getInitLogByType(sFileName, type);
}

bool CLogManager::checkLogLevel(MfwLogLevel level, bool bIsFramework)
{
    return bIsFramework ? (level >= m_mfwFrameworkLogLevel) : (level >= m_mfwLogLevel);
}

MfwLogType CLogManager::getLogType(CLog *log)
{
    if (log == NULL || log == &g_dummyStdoutLog) {
        return MfwLogType_Roll;
    }
    return log->getLogType();
}

const string &CLogManager::getLogFileName(CLog *log)
{
    if (log == NULL || log == &g_dummyStdoutLog) {
        return UtilString::getEmptyString();
    }
    return log->getFileName();
}

void CLogManager::addLog(CLog *log, const string &sData)
{
    if (log != NULL) {
        if (log == &g_dummyStdoutLog) {
            cout << "[COUT]|" << UtilTime::formatTime(UtilTime::getNow()) << "|" << sData;
            return;
        }

        CLockGuard<CMutex> lock(m_mfwLogMutex);
        if (!m_mfwLogEnabled) {
            return;
        }

        MfwLogData empty;
        m_listLogData.push_back(empty);

        MfwLogData &data = m_listLogData.back();
        data.log = log;
        data.iTime = UtilTime::getNow();
        data.sData = sData;
    }
}

string CLogManager::filterLogData(const string &sData, uint32_t iMaxSize)
{
    string sResult;
    for (unsigned i = 0; i < sData.size(); ++i) {
        char c = sData[i];
        if (c == '|' || c == '\r' || c == '\n' || iscntrl(c)) {
            continue;
        }
        sResult.push_back(c);
        if (iMaxSize != 0 && sResult.size() >= iMaxSize) {
            break;
        }
    }
    return sResult;
}

string CLogManager::escapeLogData(const string &sData, uint32_t iMaxSize)
{
    string sResult;
    for (unsigned i = 0; i < sData.size(); ++i) {
        char c = sData[i];
        switch (c) {
        case '\\':
            sResult.append("\\\\");
            break;
        case '\r':
            sResult.append("\\r");
            break;
        case '\n':
            sResult.append("\\n");
            break;
        default: {
            if (c == '|') {
                sResult.append("\\x7c");
            } else {
                sResult.push_back(c);
            }
        }
        break;
        }
        if (iMaxSize != 0 && sResult.size() >= iMaxSize) {
            break;
        }
    }
    return sResult;
}

string CLogManager::unescapeLogData(const string &sData)
{
    return UtilEncode::c_unescape(sData);
}

}
