#include "AsyncLogger.h"
#include "util/util_time.h"
#include "util/util_string.h"
#include "util/util_file.h"

#define LOGFILE_EXPIRE_TIME	120
#define LOGFILE_CHECK_EXPIRE_INTERVAL 60

struct ReportData {
    string sIp;
    vector<mfw::LogDataItem> vDataItem;
};

typedef tr1::shared_ptr<ReportData> ReportDataPtr;

struct FileDesc {
    string	sFileIdentifier;
    string	sAppName;
    string	sServerName;
    string	sDivision;

    bool operator<(const FileDesc &b) const
    {
        if (sFileIdentifier != b.sFileIdentifier) return sFileIdentifier < b.sFileIdentifier;
        if (sAppName != b.sAppName) return sAppName < b.sAppName;
        if (sServerName != b.sServerName) return sServerName < b.sServerName;
        return sDivision < b.sDivision;
    }
};

struct FileAccessInfo {
    FILE *fp;
    uint32_t iAccessTime;
};

static map<FileDesc, list<ReportDataPtr> > g_mFile2ReportData;
static CMutex g_mutexReportData;

static map<FileDesc, FileAccessInfo> g_mFile2AccessInfo;

void addToAsyncLogger(const mfw::LogBaseInfo &info, const map<string, vector<mfw::LogDataItem> > &mFileData, const string &sIp)
{
    if (mFileData.empty()) {
        return;
    }

    FileDesc stFileDesc;
    stFileDesc.sAppName = info.sAppName;
    stFileDesc.sServerName = info.sServerName;
    stFileDesc.sDivision = info.sDivision;

    CLockGuard<CMutex> lock(g_mutexReportData);
    for (map<string, vector<mfw::LogDataItem> >::const_iterator first = mFileData.begin(), last = mFileData.end(); first != last; ++first) {
        stFileDesc.sFileIdentifier = first->first;

        ReportDataPtr pReportData(new ReportData);
        pReportData->sIp = sIp;
        pReportData->vDataItem = first->second;

        g_mFile2ReportData[stFileDesc].push_back(pReportData);
    }
}

void checkCloseExpiredFile(uint32_t iNow)
{
    for (map<FileDesc, FileAccessInfo>::iterator it = g_mFile2AccessInfo.begin(); it != g_mFile2AccessInfo.end(); ) {
        FileAccessInfo &stFileAccessInfo = it->second;
        if (stFileAccessInfo.iAccessTime + LOGFILE_EXPIRE_TIME < iNow) {
            fclose(stFileAccessInfo.fp);
            g_mFile2AccessInfo.erase(it++);
        } else {
            ++it;
        }
    }
}

string buildFileName(const FileDesc &stFileDesc)
{
    string sFileName = g_stLogServerConfig.sLogPath + "/" + stFileDesc.sAppName + "/";
    if (!stFileDesc.sDivision.empty()) {
        sFileName += stFileDesc.sDivision + "/";
    }
    sFileName += stFileDesc.sServerName + "/";
    sFileName += stFileDesc.sAppName + "." + stFileDesc.sServerName + "_" + stFileDesc.sFileIdentifier + ".log";
    return sFileName;
}

void flushLogToDisk(uint32_t iNow)
{
    map<FileDesc, list<ReportDataPtr> > mFile2ReportData;
    CLockGuard<CMutex> lock(g_mutexReportData);
    swap(g_mFile2ReportData, mFile2ReportData);
    lock.unlock();

    for (map<FileDesc, list<ReportDataPtr> >::iterator first = mFile2ReportData.begin(), last = mFile2ReportData.end(); first != last; ++first) {
        const FileDesc &stFileDesc = first->first;
        list<ReportDataPtr> &listReportData = first->second;

        map<FileDesc, FileAccessInfo>::iterator it = g_mFile2AccessInfo.find(stFileDesc);
        if (it == g_mFile2AccessInfo.end()) {
            string sFileName = buildFileName(stFileDesc);
            UtilFile::makeDirectoryRecursive(UtilFile::getFileDirname(sFileName));
            FILE *fp = fopen(sFileName.c_str(), "a+");
            if (fp == NULL) {
                LOG_ERROR("cannot open file: " << sFileName);
                continue;
            }

            FileAccessInfo &stFileAccessInfo = g_mFile2AccessInfo[stFileDesc];
            stFileAccessInfo.fp = fp;
            stFileAccessInfo.iAccessTime = iNow;
            it = g_mFile2AccessInfo.find(stFileDesc);
        }

        FileAccessInfo &stFileAccessInfo = it->second;
        FILE *fp = stFileAccessInfo.fp;
        for (list<ReportDataPtr>::iterator first = listReportData.begin(), last = listReportData.end(); first != last; ++first) {
            ReportData &stReportData = **first;
            for (unsigned i = 0; i < stReportData.vDataItem.size(); ++i) {
                mfw::LogDataItem &data = stReportData.vDataItem[i];
                string sTime = UtilTime::formatTime(data.iTime, "|%Y-%m-%d %H:%M:%S|");
                fwrite(stReportData.sIp.c_str(), 1, stReportData.sIp.size(), fp);
                fwrite(sTime.c_str(), 1, sTime.size(), fp);
                fwrite(data.sData.c_str(), 1, data.sData.size(), fp);
                if (data.sData.empty() || data.sData[data.sData.size() - 1] != '\n') {
                    fwrite("\n", 1, 1, fp);
                }
            }
        }
        fflush(fp);
        stFileAccessInfo.iAccessTime = iNow;
    }
}

void asyncLoggerThread(CThread &thread)
{
    uint32_t iLastCheckTime = UtilTime::getNow();
    while (true) {
        uint32_t iNow = UtilTime::getNow();
        if (iLastCheckTime + LOGFILE_CHECK_EXPIRE_INTERVAL < iNow) {
            checkCloseExpiredFile(iNow);
            iLastCheckTime = iNow;
        }

        flushLogToDisk(iNow);

        if (thread.isTerminate()) {
            break;
        }
        thread.timedwait(1000);
    }
}
