#ifndef _MFW_UTIL_LOG_
#define _MFW_UTIL_LOG_

#include <stdint.h>
#include <string>
#include <list>
#include <map>
#include <sstream>
#include <stdexcept>
#include "util_thread.h"
#include "util_singleton.h"
using namespace std;

namespace mfw
{

enum MfwLogLevel
{
	MfwLogLevel_Debug,
	MfwLogLevel_Info,
	MfwLogLevel_Error,
	MfwLogLevel_None,
};

enum MfwLogType
{
	MfwLogType_Roll,
	MfwLogType_Day,
	MfwLogType_Hour,
	MfwLogType_GlobalDay,
	MfwLogType_GlobalHour,
	MfwLogType_LocalDay,
	MfwLogType_LocalHour,
	MfwLogType_LocalGlobalDay,
	MfwLogType_LocalGlobalHour,
};

class CLog;

struct MfwLogData
{
	CLog		*log;
	uint32_t	iTime;
	string		sData;
};

class CLogManager : public CSingleton<CLogManager>
{
public:
    CLogManager();
	void initLog(const string &sLogPath, const string &sLogPrefix);
	void finiLog();
	void setRemoteCallback(tr1::function<void(list<MfwLogData> &)> cb);
	void setRollLogInfo(CLog *log, uint32_t iMaxFile, uint64_t iMaxSize);
	void setLogLevel(MfwLogLevel level, MfwLogLevel frameworkLevel);
	void setLogLevel(const string &sLevel, const string &sFrameworkLevel);

	CLog *getRollLog(const string &sFileName = "");
	CLog *getDayLog(const string &sFileName, bool bGlobal = false, bool bLocal = false);
	CLog *getHourLog(const string &sFileName, bool bGlobal = false, bool bLocal = false);
	bool checkLogLevel(MfwLogLevel level, bool bIsFramework);
	MfwLogType getLogType(CLog *log);
	const string &getLogFileName(CLog *log);
	void addLog(CLog *log, const string &sData);

	string filterLogData(const string &sData, uint32_t iMaxSize = 0);
	string escapeLogData(const string &sData, uint32_t iMaxSize = 0);
	string unescapeLogData(const string &sData);

private:
    void cleanupMfwLog();
    void loggerThreadEntry(CThread &thread);
    CLog *getInitLogByType(const string &sFileName, MfwLogType type);

private:
    string m_mfwLogPath;
    string m_mfwLogPrefix;
    MfwLogLevel m_mfwLogLevel;
    MfwLogLevel m_mfwFrameworkLogLevel;
    CMutex m_mfwLogMutex;
    CThread m_mfwLogThread;
    map<string, CLog *> m_mAllMfwLog;
    list<MfwLogData> m_listLogData;
    bool m_mfwLogEnabled;
    tr1::function<void(list<MfwLogData> &)> m_remoteCallback;
};

const char *briefLogFileName(const char *name);

#define MFWLOG_WITH_LEVELCHECK(level, bIsFramework, x) do \
	{ \
		using namespace mfw; \
		CLog *log = CLogManager::getInstance()->getRollLog(); \
		if (log != NULL && CLogManager::getInstance()->checkLogLevel(level, bIsFramework)) \
		{ \
			ostringstream s1s2; \
			s1s2 << UtilThread::getThreadId() << "|" << briefLogFileName(__FILE__) << ":" << __LINE__ << ":" << __FUNCTION__ << "|" << x << endl; \
			CLogManager::getInstance()->addLog(log, s1s2.str()); \
		} \
	} while (0)

#define MFW_DEBUG(x) MFWLOG_WITH_LEVELCHECK(MfwLogLevel_Debug, true, "DEBUG|[MFW]" << x)
#define MFW_INFO(x) MFWLOG_WITH_LEVELCHECK(MfwLogLevel_Info, true, "INFO|[MFW]" << x)
#define MFW_ERROR(x) MFWLOG_WITH_LEVELCHECK(MfwLogLevel_Error, true, "ERROR|[MFW]" << x)

#define LOG_DEBUG(x) MFWLOG_WITH_LEVELCHECK(MfwLogLevel_Debug, false, "DEBUG|" << x)
#define LOG_INFO(x) MFWLOG_WITH_LEVELCHECK(MfwLogLevel_Info, false, "INFO|" << x)
#define LOG_ERROR(x) MFWLOG_WITH_LEVELCHECK(MfwLogLevel_Error, false, "ERROR|" << x)

#define DAYLOG(file, x) do \
	{ \
		using namespace mfw; \
		CLog *log = CLogManager::getInstance()->getDayLog(file); \
		if (log != NULL) \
		{ \
			ostringstream s1s2; \
			s1s2 << x << endl; \
			CLogManager::getInstance()->addLog(log, s1s2.str()); \
		} \
	} while (0)

#define LOCAL_DAYLOG(file, x) do \
	{ \
		using namespace mfw; \
		CLog *log = CLogManager::getInstance()->getDayLog(file, false, true); \
		if (log != NULL) \
		{ \
			ostringstream s1s2; \
			s1s2 << x << endl; \
			CLogManager::getInstance()->addLog(log, s1s2.str()); \
		} \
	} while (0)


#define GLOBAL_DAYLOG(file, x) do \
	{ \
		using namespace mfw; \
		CLog *log = CLogManager::getInstance()->getDayLog(file, true); \
		if (log != NULL) \
		{ \
			ostringstream s1s; \
			s1s << x << endl; \
			CLogManager::getInstance()->addLog(log, s1s.str()); \
		} \
	} while (0)

#define LOCAL_GLOBAL_DAYLOG(file, x) do \
	{ \
		using namespace mfw; \
		CLog *log = CLogManager::getInstance()->getDayLog(file, true, true); \
		if (log != NULL) \
		{ \
			ostringstream s1s; \
			s1s << x << endl; \
			CLogManager::getInstance()->addLog(log, s1s.str()); \
		} \
	} while (0)

#define HOURLOG(file, x) do \
	{ \
		using namespace mfw; \
		CLog *log = CLogManager::getInstance()->getHourLog(file); \
		if (log != NULL) \
		{ \
			ostringstream s1s; \
			s1s << x << endl; \
			CLogManager::getInstance()->addLog(log, s1s.str()); \
		} \
	} while (0)

#define LOCAL_HOURLOG(file, x) do \
	{ \
		using namespace mfw; \
		CLog *log = CLogManager::getInstance()->getHourLog(file, false, true); \
		if (log != NULL) \
		{ \
			ostringstream s1s; \
			s1s << x << endl; \
			CLogManager::getInstance()->addLog(log, s1s.str()); \
		} \
	} while (0)

#define GLOBAL_HOURLOG(file, x) do \
	{ \
		using namespace mfw; \
		CLog *log = CLogManager::getInstance()->getHourLog(file, true); \
		if (log != NULL) \
		{ \
			ostringstream s1s; \
			s1s << x << endl; \
			CLogManager::getInstance()->addLog(log, s1s.str()); \
		} \
	} while (0)

#define LOCAL_GLOBAL_HOURLOG(file, x) do \
	{ \
		using namespace mfw; \
		CLog *log = CLogManager::getInstance()->getHourLog(file, true, true); \
		if (log != NULL) \
		{ \
			ostringstream s1s; \
			s1s << x << endl; \
			CLogManager::getInstance()->addLog(log, s1s.str()); \
		} \
	} while (0)

#define LOG_DAYERROR(x) DAYLOG("error", x)
#define LOG_EXCEPTION(x) DAYLOG("exception", UtilThread::getThreadId() << "|" << briefLogFileName(__FILE__) << ":" << __LINE__ << ":" << __FUNCTION__ << "|" << x)

#define __TRY__ try \
	{

#define __CATCH__ \
	} \
	catch (std::exception &e) \
	{ \
		LOG_EXCEPTION(e.what()); \
		LOG_ERROR(e.what()); \
	} \
	catch (...) \
	{ \
		LOG_EXCEPTION("unknown exception"); \
	}

}

#endif
