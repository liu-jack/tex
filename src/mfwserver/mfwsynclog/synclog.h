#ifndef _SYNC_LOG_H_
#define _SYNC_LOG_H_

#include <stdint.h>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <tr1/memory>
#include "util/util_mysql.h"
#include "util/util_noncopyable.h"
#include "tinyxml/tinyxml.h"

using namespace std;
using namespace mfw;

enum LogFileType {
    LogFileType_Hour,
    LogFileType_Day,
};

enum FieldDataType {
    FieldDataType_String,
    FieldDataType_DateTime,
    FieldDataType_Int32,
    FieldDataType_UInt32,
    FieldDataType_Int64,
    FieldDataType_UInt64,
    FieldDataType_Char,
    FieldDataType_Float,
};

enum FieldParserType {
    FiledParserType_None,
    FieldParserType_Vector,
    FieldParserType_Map,
};

struct LogFieldConfig {
    string			sDesc;					// 列名描述
    uint32_t		iFieldIndex;			// 日志格式索引(从0开始)
    string			sMatch;					// 日志匹配字符串
    string			sColumn;				// 列名
    uint32_t		iFieldDataType;			// 数据类型
    bool			bIndex;					// 是否增加索引列

    FieldParserType emFieldParserType;		// 列解析类型
    string 			sParserSep;				// 列解析分隔符
    vector<LogFieldConfig> vParserField; 	// 列解析结果

    string getInsertFields() const;
    string getInsertValues(CMysql &mysql, const string &sValue) const;
    string getCreateFields() const;

    void getAllDBColumns(vector<LogFieldConfig> &vDBColumn) const;

    int32_t setFieldType(const string &sDataType);
};

struct LogTableConfig {
    string			sName;
    string			sDesc;
    vector<LogFieldConfig> vFieldConfig;
    vector<uint32_t>	vMatchFieldIndex;

    void getAllDBColumns(vector<LogFieldConfig> &vDBColumn) const;
};

struct LogFileConfig {
    string			sLogFileName;
    LogFileType		emLogFileType;
    vector<LogTableConfig>	vLogTableConfig;
};

struct SyncConfig {
    map<string, string>		mVariable;
    string					sLogPath;
    string					sProgressFile;
    string					sSyncLogPath;
    string					sSyncLogPrefix;
    uint32_t				iSyncInterval;
    uint32_t				iLinesPerLoop;
    CDBConf					stDBConf;
    vector<LogFileConfig>	vLogFileConfig;
};

class CTableConfigFinder
{
    struct FindNode {
        vector<uint32_t> vIndex;
        map<string, const LogTableConfig *> mConfig; // match组合的字符串(\0分隔)
    };

public:
    CTableConfigFinder() : m_pGlobTableConfig(NULL) {}
    void init(const vector<LogTableConfig> &vLogTableConfig);
    const LogTableConfig *findConfig(const vector<string> &vField);

private:
    map<string, FindNode> m_mFindNode; // index组合的字符串(,分隔) => 查找信息
    const LogTableConfig *m_pGlobTableConfig;
};

struct ProgressInfo {
    string		sLogFileName;
    string		sDateHour;
    uint64_t	iOffset;
};

class CProgressManager : public mfw::noncopyable
{
public:
    int32_t init(const string &sProgresFile);
    ProgressInfo getProgress(const string &sLogFileName) const;
    void saveProgress(const ProgressInfo &stProgressInfo);

private:
    string		m_sProgressFile;
    map<string, ProgressInfo> m_mProgressInfo;
};

typedef tr1::shared_ptr<ostringstream> ostringstreamptr;

class CSqlBuilder
{
public:
    explicit CSqlBuilder(CMysql &mysql) : m_mysql(mysql) {}
    void addLine(const LogTableConfig &stLogTableConfig, const vector<string> &vField);

public:
    map<string, ostringstreamptr> m_mTableSql;

private:
    CMysql &m_mysql;
};

class CFileLoader : public mfw::noncopyable
{
public:
    int32_t init(const string &sLogPath, const string &sLogFileName, LogFileType emLogFileType, const ProgressInfo &stProgressInfo);
    int32_t getLine(vector<string> &vField);
    void resetProgress(const ProgressInfo &stProgressInfo);
    const ProgressInfo &getProgress() const
    {
        return m_stProgressInfo;
    }
    const string &getLogFileName() const
    {
        return m_sLogFileName;
    }

private:
    int32_t scanFile(bool &bFileChanged);
    bool seekNextFile();
    bool getNextFile(string &sDateHour);
    bool findLine(vector<string> &vField);

private:
    string			m_sLogPath;
    string			m_sLogFileName;
    LogFileType		m_emLogFileType;
    ProgressInfo	m_stProgressInfo;

    FILE			*m_fp;
    string			m_sReadBuf;
};

typedef tr1::shared_ptr<CFileLoader> CFileLoaderPtr;

class CSyncLog : public mfw::noncopyable
{
public:
    CSyncLog() : m_bDisableReplaceVariable(false) {}
    int32_t loadConfig(const string &sSyncConfig, const string &sSkelConfig);
    int32_t prepareDatabase(bool bAutoCreateDb = false);
    int32_t prepareFileLoader();
    int32_t processLog();

    int32_t printSkel(const string &sSkelConfig, const string &sPrintFile);

    void setVariable(const map<string, string> &mVariable)
    {
        m_stSyncConfig.mVariable = mVariable;
    }

private:
    int32_t loadSyncConfig(const string &sConfigFile);
    int32_t loadSkelConfig(const string &sConfigFile);
    int32_t buildTableConfigFinder();
    string replaceVariable(const string &sData);

    int32_t getAllTableInDB(set<string> &setTable);
    int32_t createNewTable(const LogTableConfig &stLogTableConfig);
    int32_t checkAndAlterTable(const LogTableConfig &stLogTableConfig);

    int32_t syncOneFile(CFileLoader &loader);
    int32_t parseLogFieldConfig(TiXmlElement *node, LogFieldConfig &stLogFieldConfig, int32_t &iFieldIndex);
    int32_t parseImportFieldConfig(TiXmlElement *root, TiXmlElement *node, vector<LogFieldConfig> &vLogFieldConfig, int32_t &iFieldIndex);

private:
    bool m_bDisableReplaceVariable;
    SyncConfig	m_stSyncConfig;
    map<string, CTableConfigFinder> m_mTableConfigFinder;
    CMysql		m_mysql;
    CProgressManager	m_progressManager;
    map<string, CFileLoaderPtr> m_mFileLoaderPtr;
};

extern bool g_bQuitApplicaiton;

#endif
