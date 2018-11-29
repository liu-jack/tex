#include "synclog.h"
#include <iostream>
#include <stdexcept>
#include <algorithm>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include "util/util_log.h"
#include "util/util_string.h"
#include "util/util_file.h"
#include "util/util_time.h"
#include "util/util_arith.h"

#define ERROR_RET(msg) do { cout << __LINE__ << ":" << msg << endl; LOG_ERROR(msg); return -1; } while (0)

bool g_bQuitApplicaiton = false;

string getSqlFieldType(uint32_t iFieldDataType)
{
	FieldDataType emFieldDataType = static_cast<FieldDataType>(iFieldDataType & 0x0000FFFF);
	uint32_t iLength = (iFieldDataType & 0xFFFF0000) >> 16;
	switch (emFieldDataType)
	{
	case FieldDataType_String: return "VARCHAR(256)";
	case FieldDataType_DateTime: return "DATETIME";
	case FieldDataType_Int32: return "INT";
	case FieldDataType_UInt32: return "INT UNSIGNED";
	case FieldDataType_Int64: return "BIGINT";
	case FieldDataType_UInt64: return "BIGINT UNSIGNED";
	case FieldDataType_Char:
		{
			return "CHAR(" + UtilString::tostr(iLength) + ")";
		}
    case FieldDataType_Float: return "float";
	}
	return "";
}

string LogFieldConfig::getInsertValues(CMysql &mysql, const string &sValue) const
{
	ostringstream os;

	bool bFirst = true;

	if (!sColumn.empty())
	{
		FieldDataType emFieldDataType = static_cast<FieldDataType>(iFieldDataType & 0x0000FFFF);
		switch (emFieldDataType)
		{
		case FieldDataType_String: os << "\"" << mysql.escape(sValue) << "\""; break;
		case FieldDataType_DateTime: os << (!sValue.empty() ? ("\"" + sValue + "\"") : "NULL"); break;
		case FieldDataType_Int32: os << UtilString::strto<int32_t>(sValue); break;
		case FieldDataType_UInt32: os << UtilString::strto<uint32_t>(sValue); break;
		case FieldDataType_Int64: os << UtilString::strto<int64_t>(sValue); break;
		case FieldDataType_UInt64: os << UtilString::strto<uint64_t>(sValue); break;
		case FieldDataType_Char: os << "\"" << mysql.escape(sValue) << "\""; break;
        case FieldDataType_Float: os << UtilString::strto<float>(sValue); break;
		}

		bFirst = false;
	}

	if (emFieldParserType == FieldParserType_Vector)
	{
		vector<string> vField;
		UtilString::splitString(sValue, sParserSep, false, vField);
		for (uint32_t i = 0; i < vParserField.size(); ++i)
		{
			string result = vParserField[i].getInsertValues(mysql, (i < vField.size() ? vField[i] : ""));
			if (result.empty()) continue;
			
			if (!bFirst) os << ",";
			else bFirst = false;
			os << result;
		}
	} 

	return os.str();
}

string LogFieldConfig::getCreateFields() const
{
	ostringstream os;
	
	bool bFirst = true;
	if (!sColumn.empty())
	{
		os << sColumn << " " << getSqlFieldType(iFieldDataType);
		if (bIndex) os << ", " << "INDEX (" << sColumn << ")";
		bFirst = false;
	}

	for (uint32_t i = 0; i < vParserField.size(); ++i)
	{
		string result = vParserField[i].getCreateFields();
		if (result.empty()) continue;

		if (!bFirst)
		{
			os << ",";
		}
		else
		{
			bFirst = false;
		}

		os << result;
	}

	return os.str();
}

void LogFieldConfig::getAllDBColumns(vector<LogFieldConfig> &vDBColumn) const
{
	if (!sColumn.empty()) vDBColumn.push_back(*this);

	for (uint32_t i = 0; i < vParserField.size(); ++i)
	{
		vParserField[i].getAllDBColumns(vDBColumn);
	}
}

int32_t LogFieldConfig::setFieldType(const string &sFieldDataType)
{
	if (sFieldDataType == "str") iFieldDataType = FieldDataType_String;
	else if (sFieldDataType == "datetime") iFieldDataType = FieldDataType_DateTime;
	else if (sFieldDataType == "int32") iFieldDataType = FieldDataType_Int32;
	else if (sFieldDataType == "uint32") iFieldDataType = FieldDataType_UInt32;
	else if (sFieldDataType == "int64") iFieldDataType = FieldDataType_Int64;
	else if (sFieldDataType == "uint64") iFieldDataType = FieldDataType_UInt64;
	else if (sFieldDataType == "float") iFieldDataType = FieldDataType_Float;
	else if (UtilString::isBeginsWith(sFieldDataType, "char("))
	{
		iFieldDataType = FieldDataType_Char;
		unsigned int iLength = 0;
		sscanf(sFieldDataType.c_str(), "char(%u)", &iLength);
		if (iLength > 255)
		{
			ERROR_RET("error: invalid root/file/log@type, " << sDesc << "," << sColumn << "," << sFieldDataType);
		}
		iFieldDataType |= (iLength << 16);
	}
	else
		ERROR_RET("error: invalid root/file/log@type, " << sDesc << "," << sColumn << "," << sFieldDataType);

	return 0;
}

void LogTableConfig::getAllDBColumns(vector<LogFieldConfig> &vDBColumn) const
{
	for (uint32_t i = 0; i < vFieldConfig.size(); ++i)
	{
		vFieldConfig[i].getAllDBColumns(vDBColumn);
	}
}

void CTableConfigFinder::init(const vector<LogTableConfig> &vLogTableConfig)
{
	for (unsigned i = 0; i < vLogTableConfig.size(); ++i)
	{
		const LogTableConfig &stLogTableConfig = vLogTableConfig[i];
		if (stLogTableConfig.vMatchFieldIndex.empty())
		{
			m_pGlobTableConfig = &stLogTableConfig;
			continue;
		}

		string sIndex, sMatch;
		for (unsigned i = 0; i < stLogTableConfig.vMatchFieldIndex.size(); ++i)
		{
			if (i != 0)
			{
				sIndex += ",";
				sMatch += "\0";
			}
			uint32_t iIndex = stLogTableConfig.vMatchFieldIndex[i];
			sIndex += UtilString::tostr(iIndex);
			sMatch += stLogTableConfig.vFieldConfig[iIndex].sMatch;
		}

		map<string, FindNode>::iterator it = m_mFindNode.find(sIndex);
		if (it == m_mFindNode.end())
		{
			FindNode &stFindNode = m_mFindNode[sIndex];
			stFindNode.vIndex = stLogTableConfig.vMatchFieldIndex;
			it = m_mFindNode.find(sIndex);
		}

		FindNode &stFindNode = it->second;
		stFindNode.mConfig[sMatch] = &stLogTableConfig;
	}
}

const LogTableConfig *CTableConfigFinder::findConfig(const vector<string> &vField)
{
	for (map<string, FindNode>::const_iterator first = m_mFindNode.begin(), last = m_mFindNode.end(); first != last; ++first)
	{
		const FindNode &stFindNode = first->second;

		string sMatch;
		for (unsigned i = 0; i < stFindNode.vIndex.size(); ++i)
		{
			uint32_t iIndex = stFindNode.vIndex[i];
			if (i != 0)
			{
				sMatch += "\0";
			}
			if (iIndex < vField.size())
			{
				sMatch += vField[iIndex];
			}
		}

		map<string, const LogTableConfig *>::const_iterator it = stFindNode.mConfig.find(sMatch);
		if (it != stFindNode.mConfig.end())
		{
			return it->second;
		}
	}
	return m_pGlobTableConfig;
}

int32_t CProgressManager::init(const string &sProgresFile)
{
	m_sProgressFile = sProgresFile;

	string sContent = UtilFile::loadFromFile(sProgresFile);
	vector<string> vLines = UtilString::splitString(sContent, "\r\n", true);
	for (unsigned i = 0; i < vLines.size(); ++i)
	{
		const string &sLine = vLines[i];
		vector<string> vData = UtilString::splitString(sLine, " \t", true);
		if (vData.size() != 3)
		{
			ERROR_RET("progress file format: " << sLine);
		}

		ProgressInfo &stProgressInfo = m_mProgressInfo[vData[0]];
		stProgressInfo.sLogFileName = vData[0];
		stProgressInfo.sDateHour = vData[1];
		stProgressInfo.iOffset = UtilString::strto<uint64_t>(vData[2]);
	}
	return 0;
}

ProgressInfo CProgressManager::getProgress(const string &sLogFileName) const
{
	map<string, ProgressInfo>::const_iterator it = m_mProgressInfo.find(sLogFileName);
	if (it == m_mProgressInfo.end())
	{
		ProgressInfo stProgressInfo;
		stProgressInfo.sLogFileName = sLogFileName;
		stProgressInfo.sDateHour = "";
		stProgressInfo.iOffset = 0;
		return stProgressInfo;
	}
	return it->second;
}

void CProgressManager::saveProgress(const ProgressInfo &stProgressInfo)
{
	m_mProgressInfo[stProgressInfo.sLogFileName] = stProgressInfo;

	ostringstream os;
	for (map<string, ProgressInfo>::const_iterator first = m_mProgressInfo.begin(), last = m_mProgressInfo.end(); first != last; ++first)
	{
		const ProgressInfo &stProgressInfo = first->second;
		os << stProgressInfo.sLogFileName << "\t" << stProgressInfo.sDateHour << "\t" << stProgressInfo.iOffset << endl;
	}

	UtilFile::saveToFile(m_sProgressFile, os.str());
}

void CSqlBuilder::addLine(const LogTableConfig &stLogTableConfig, const vector<string> &vField)
{
	bool bFirst = false;
	ostringstreamptr &ptr = m_mTableSql[stLogTableConfig.sName];
	if (!ptr)
	{
		ptr = ostringstreamptr(new ostringstream);
		bFirst = true;
	}

	ostringstream &os = *ptr;
	if (bFirst)
	{
		os << "INSERT INTO " << stLogTableConfig.sName << " (";
		vector<LogFieldConfig> vDBColumn;
		stLogTableConfig.getAllDBColumns(vDBColumn);
		for (unsigned i = 0; i < vDBColumn.size(); ++i)
		{
			const LogFieldConfig &stLogFieldConfig = vDBColumn[i];
			if (i != 0)
			{
				os << ",";
			}
			os << stLogFieldConfig.sColumn;
		}
		os << ") VALUES ";
	}
	else
	{
		os << ",";
	}

	os << "(";
	bFirst = true;
	for (unsigned i = 0; i < stLogTableConfig.vFieldConfig.size(); ++i)
	{
		const LogFieldConfig &stLogFieldConfig = stLogTableConfig.vFieldConfig[i];

		const string *pval = NULL;
		if (stLogFieldConfig.iFieldIndex < vField.size())
		{
			pval = &vField[stLogFieldConfig.iFieldIndex];
		}
		else
		{
			pval = &UtilString::getEmptyString();
		}

		const string &sValue = *pval;
		string result = stLogFieldConfig.getInsertValues(m_mysql, sValue);
		if (result.empty()) continue;

		if (!bFirst) os << ",";
		else bFirst = false;

		os << result;
	}
	os << ")";
}

int32_t CFileLoader::init(const string &sLogPath, const string &sLogFileName, LogFileType emLogFileType, const ProgressInfo &stProgressInfo)
{
	m_sLogPath = sLogPath;
	m_sLogFileName = sLogFileName;
	m_emLogFileType = emLogFileType;
	m_stProgressInfo = stProgressInfo;

	m_fp = NULL;
	return 0;
}

int32_t CFileLoader::getLine(vector<string> &vField)
{
	int32_t ret = 0;

	if (m_fp == NULL)
	{
		// 开始第一个文件
		bool bFileChanged = false;
		ret = scanFile(bFileChanged);
		if (ret != 0)
		{
			return ret;
		}
		if (!bFileChanged)
		{
			return 0;
		}
		LOG_DEBUG(m_sLogFileName << " enter first file: " << m_stProgressInfo.sDateHour << "," << m_stProgressInfo.iOffset);
	}

	while (true)
	{
		// 放在前面执行，因为一次fread可以已经读取多条日志到buf中
		if (findLine(vField) && !vField.empty())
		{
			return 0;
		}

		char buf[4096];
		ret = fread(buf, 1, sizeof(buf), m_fp);
		if (ret <= 0)
		{
			// 当前文件读完了，如果已经有下一个文件，就清空现有的buf，转入下一个文件
			bool bFileChanged = false;
			ret = scanFile(bFileChanged);
			if (ret != 0)
			{
				return ret;
			}
			if (!bFileChanged)
			{
				// 当前文件读完了，且没有下一个文件
				LOG_DEBUG(m_sLogFileName << " has no more file");
				break;
			}
			LOG_DEBUG(m_sLogFileName << " enter next file: " << m_stProgressInfo.sDateHour << "," << m_stProgressInfo.iOffset);

			// 读下一个文件
			ret = fread(buf, 1, sizeof(buf), m_fp);
		}
		if (ret <= 0)
		{
			break;
		}

		m_sReadBuf.append(buf, ret);
	}

	return 0;
}

void CFileLoader::resetProgress(const ProgressInfo &stProgressInfo)
{
	if (m_stProgressInfo.sLogFileName != stProgressInfo.sLogFileName)
	{
		return;
	}

	bool bReset = false;
	if (m_fp == NULL)
	{
		m_stProgressInfo = stProgressInfo;
		bReset = true;
	}
	else
	{
		if (m_stProgressInfo.sDateHour == stProgressInfo.sDateHour)
		{
			if (m_stProgressInfo.iOffset != stProgressInfo.iOffset)
			{
				m_stProgressInfo = stProgressInfo;
				fseeko(m_fp, m_stProgressInfo.iOffset, SEEK_SET);
				bReset = true;
			}
		}
		else
		{
			string sFile = m_sLogPath + "/" + m_sLogFileName + "_" + stProgressInfo.sDateHour + ".log";
			FILE *fp = fopen(sFile.c_str(), "r");
			if (fp == NULL)
			{
				LOG_ERROR("cannot open log file(reset): " << sFile);
				return;
			}

			m_fp = fp;
			m_stProgressInfo = stProgressInfo;
			fseeko(m_fp, m_stProgressInfo.iOffset, SEEK_SET);
			bReset = true;
		}
	}

	if (bReset)
	{
		LOG_DEBUG("reset progress to: " << m_sLogFileName << "_" << m_stProgressInfo.sDateHour << ", offset: " << m_stProgressInfo.iOffset);
	}
}

int32_t CFileLoader::scanFile(bool &bFileChanged)
{
	bFileChanged = false;
	if (m_fp == NULL)
	{
		if (m_stProgressInfo.sDateHour.empty())
		{
			if (!seekNextFile())
			{
				// 还没有文件
				return 0;
			}
		}
		else
		{
			string sFile = m_sLogPath + "/" + m_sLogFileName + "_" + m_stProgressInfo.sDateHour + ".log";
			if (!UtilFile::isFileExists(sFile))
			{
				if (!seekNextFile())
				{
					return 0;
				}
			}
		}

		string sFile = m_sLogPath + "/" + m_sLogFileName + "_" + m_stProgressInfo.sDateHour + ".log";
		m_fp = fopen(sFile.c_str(), "r");
		if (m_fp == NULL)
		{
			LOG_ERROR("cannot open log file: " << sFile);
		}

		if (m_stProgressInfo.iOffset != 0)
		{
			fseeko(m_fp, m_stProgressInfo.iOffset, SEEK_SET);
		}
		bFileChanged = true;
	}
	else
	{
		if (!seekNextFile())
		{
			return 0;
		}

		// 切换到下一个文件
		fclose(m_fp);
		m_fp = NULL;
		m_sReadBuf.clear();

		string sFile = m_sLogPath + "/" + m_sLogFileName + "_" + m_stProgressInfo.sDateHour + ".log";
		m_fp = fopen(sFile.c_str(), "r");
		if (m_fp == NULL)
		{
			LOG_ERROR("cannot open log file: " << sFile);
		}
		bFileChanged = true;
	}
	return 0;
}

bool CFileLoader::seekNextFile()
{
	string sDateHour;
	if (!getNextFile(sDateHour))
	{
		return false;
	}
	m_stProgressInfo.sDateHour = sDateHour;
	m_stProgressInfo.iOffset = 0;
	// LOG_DEBUG(m_sLogFileName << " seek next file: " << m_stProgressInfo.sDateHour << "," << m_stProgressInfo.iOffset);
	return true;
}

bool CFileLoader::getNextFile(string &sDateHour)
{
	vector<string> vFiles;
	UtilFile::listDirectory(m_sLogPath, vFiles, false, false);
	sort(vFiles.begin(), vFiles.end());

	uint32_t iCurDate = 0, iCurHour = 0;
	if (!m_stProgressInfo.sDateHour.empty())
	{
		if (m_emLogFileType == LogFileType_Hour)
		{
			sscanf(m_stProgressInfo.sDateHour.c_str(), "%u_%u", &iCurDate, &iCurHour);
		}
		else
		{
			sscanf(m_stProgressInfo.sDateHour.c_str(), "%u", &iCurDate);
		}
	}

	for (unsigned i = 0; i < vFiles.size(); ++i)
	{
		const string &sFile = vFiles[i];
		if (!UtilString::isBeginsWith(sFile, m_sLogFileName))
		{
			continue;
		}
		// LOG_DEBUG(m_sLogFileName << " compare file: " << sFile);

		const char *part = sFile.c_str() + m_sLogFileName.size();
		uint32_t iNextDate = 0, iNextHour = 0;
		int iBytes = 0;
		if (m_emLogFileType == LogFileType_Hour)
		{
			if (sscanf(part, "_%u_%u%n", &iNextDate, &iNextHour, &iBytes) != 2)
			{
				continue;
			}
		}
		else
		{
			if (sscanf(part, "_%u%n", &iNextDate, &iBytes) != 1)
			{
				continue;
			}
		}

		if (strcmp(part + iBytes, ".log") != 0)
		{
			continue;
		}

		if (iNextDate > iCurDate || (iNextDate == iCurDate && iNextHour > iCurHour))
		{
			if (m_emLogFileType == LogFileType_Hour)
			{
				sDateHour = UtilString::format("%04u_%02u", iNextDate, iNextHour);
			}
			else
			{
				sDateHour = UtilString::format("%04u", iNextDate);
			}
			return true;
		}
	}
	return false;
}

bool CFileLoader::findLine(vector<string> &vField)
{
	string::size_type pos = m_sReadBuf.find('\n');
	if (pos == string::npos)
	{
		return false;
	}

	// 忽略日志中的IP字段
	string::size_type start = 0;
	int a, b, c, d;
	if (sscanf(m_sReadBuf.c_str(), "%d.%d.%d.%d", &a, &b, &c, &d) == 4)
	{
		start = m_sReadBuf.find('|');
		if (start == string::npos || start >= pos)
		{
			start = pos;
		}
		else
		{
			start = start + 1;
		}
	}

	string sLine = m_sReadBuf.substr(start, pos - start);
	UtilString::splitString(sLine, "|", false, vField);

	m_sReadBuf.erase(0, pos + 1);
	m_stProgressInfo.iOffset += pos + 1;
	// LOG_DEBUG(m_sLogFileName << " find line: " << sLine);
	return true;
}

int32_t CSyncLog::loadConfig(const string &sSyncConfig, const string &sSkelConfig)
{
	int32_t ret = loadSyncConfig(sSyncConfig);
	if (ret != 0)
	{
		return ret;
	}

	CLogManager::getInstance()->initLog(m_stSyncConfig.sSyncLogPath, m_stSyncConfig.sSyncLogPrefix);

	ret = loadSkelConfig(sSkelConfig);
	if (ret != 0)
	{
		return ret;
	}

	ret = buildTableConfigFinder();
	if (ret != 0)
	{
		return ret;
	}

	return 0;
}

int32_t CSyncLog::loadSyncConfig(const string &sConfigFile)
{
	TiXmlDocument doc;
	if (!doc.LoadFile(sConfigFile))
	{
		ERROR_RET("cannot open xml config file: " << sConfigFile << "," << doc.ErrorDesc());
	}

	TiXmlElement *root = doc.RootElement();
	if (root == NULL)
	{
		ERROR_RET("error: miss root");
	}

	for (TiXmlElement *variable = root->FirstChildElement("variable"); variable != NULL; variable = variable->NextSiblingElement("variable"))
	{
		string sName = variable->Attribute("name") ? : "";
		string sValue = variable->GetText() ? : "";
		if (m_stSyncConfig.mVariable.find(sName) == m_stSyncConfig.mVariable.end())
		{
			m_stSyncConfig.mVariable[sName] = sValue;
		}
	}

	TiXmlElement *logpath = root->FirstChildElement("logpath");
	if (logpath == NULL)
	{
		ERROR_RET("error: miss root/logpath");
	}
	m_stSyncConfig.sLogPath = replaceVariable(logpath->GetText() ? : "");

	TiXmlElement *progress_file = root->FirstChildElement("progress_file");
	if (progress_file == NULL)
	{
		ERROR_RET("error: miss root/progress_file");
	}
	m_stSyncConfig.sProgressFile = replaceVariable(progress_file->GetText() ? : "");
	string sProgressPath = UtilFile::getFileDirname(m_stSyncConfig.sProgressFile);
	UtilFile::makeDirectoryRecursive(UtilFile::getAbsolutePath(sProgressPath));

	TiXmlElement *synclog_path = root->FirstChildElement("synclog_path");
	if (synclog_path == NULL)
	{
		ERROR_RET("error: miss root/synclog_path");
	}
	m_stSyncConfig.sSyncLogPath = replaceVariable(synclog_path->GetText() ? : "");

	TiXmlElement *synclog_prefix = root->FirstChildElement("synclog_prefix");
	if (synclog_prefix == NULL)
	{
		ERROR_RET("error: miss root/synclog_prefix");
	}
	m_stSyncConfig.sSyncLogPrefix = replaceVariable(synclog_prefix->GetText() ? : "");

	TiXmlElement *sync_interval = root->FirstChildElement("sync_interval");
	if (sync_interval == NULL)
	{
		ERROR_RET("error: miss root/sync_interval");
	}
	m_stSyncConfig.iSyncInterval = UtilString::strto<uint32_t>(sync_interval->GetText() ? : "300");

	TiXmlElement *lines_per_loop = root->FirstChildElement("lines_per_loop");
	if (lines_per_loop == NULL)
	{
		ERROR_RET("error: miss root/lines_per_loop");
	}
	m_stSyncConfig.iLinesPerLoop = UtilString::strto<uint32_t>(lines_per_loop->GetText() ? : "1000");

	TiXmlElement *dbconf = root->FirstChildElement("dbconf");
	if (dbconf == NULL)
	{
		ERROR_RET("error: miss root/dbconf");
	}
	TiXmlElement *dbconf_host = dbconf->FirstChildElement("host");
	TiXmlElement *dbconf_port = dbconf->FirstChildElement("port");
	TiXmlElement *dbconf_db = dbconf->FirstChildElement("database");
	TiXmlElement *dbconf_user = dbconf->FirstChildElement("user");
	TiXmlElement *dbconf_password = dbconf->FirstChildElement("password");
	TiXmlElement *dbconf_charset = dbconf->FirstChildElement("charset");
	m_stSyncConfig.stDBConf.sHost = replaceVariable((dbconf_host ? dbconf_host->GetText() : NULL) ? : "");
	m_stSyncConfig.stDBConf.iPort = atoi(replaceVariable((dbconf_port ? dbconf_port->GetText() : NULL) ? : "3306").c_str());
	m_stSyncConfig.stDBConf.sDatabase = replaceVariable((dbconf_db ? dbconf_db->GetText() : NULL) ? : "");
	m_stSyncConfig.stDBConf.sUser = replaceVariable((dbconf_user ? dbconf_user->GetText() : NULL) ? : "");
	m_stSyncConfig.stDBConf.sPassword = replaceVariable((dbconf_password ? dbconf_password->GetText() : NULL) ? : "");
	m_stSyncConfig.stDBConf.sCharset = (dbconf_charset ? dbconf_charset->GetText() : NULL) ? : "utf8";
	if (m_stSyncConfig.stDBConf.sHost.empty() || m_stSyncConfig.stDBConf.sDatabase.empty())
	{
		ERROR_RET("error: db config");
	}
	return 0;
}

int32_t CSyncLog::parseLogFieldConfig(TiXmlElement *node, LogFieldConfig &stLogFieldConfig, int32_t &iFieldIndex)
{
	++iFieldIndex;

	stLogFieldConfig.sDesc = node->Attribute("desc") ? : "";
	stLogFieldConfig.iFieldIndex = iFieldIndex;
	stLogFieldConfig.sMatch = node->Attribute("match") ? : "";
	stLogFieldConfig.sColumn = node->Attribute("column") ? : "";
	int32_t index = 0;
	node->Attribute("index", &index);
	stLogFieldConfig.bIndex = index == 1;

	// 需要保存数据库
	if (!stLogFieldConfig.sColumn.empty())
	{
		string sFieldDataType = node->Attribute("type") ? : "";
		if (stLogFieldConfig.setFieldType(sFieldDataType) != 0)
		{
			return -1;
		}
	}

	// 解析成多列
	stLogFieldConfig.emFieldParserType = FiledParserType_None;
	TiXmlElement *parser = node->FirstChildElement("parser");	
	if (parser == NULL)
	{
		return 0;
	}

	string sFieldParserType = parser->Attribute("type") ? : "";	
	if (sFieldParserType == "vector")
	{
		stLogFieldConfig.emFieldParserType = FieldParserType_Vector;	
		stLogFieldConfig.sParserSep = parser->Attribute("record_sep") ? : "";
	}
	else
	{
		ERROR_RET("error: invalid root/file/log/field@parser, " << stLogFieldConfig.sDesc << "," << sFieldParserType);
	}

	int32_t iParserFieldIndex = -1;
	for (TiXmlElement *field = node->FirstChildElement("field"); field != NULL; field = field->NextSiblingElement("field"))
	{
		LogFieldConfig stParserField;
		if (parseLogFieldConfig(field, stParserField, iParserFieldIndex) != 0)
		{
			return -1;
		}

		stLogFieldConfig.vParserField.push_back(stParserField);
	}

	return 0;
}

int32_t CSyncLog::parseImportFieldConfig(TiXmlElement *root, TiXmlElement *node, vector<LogFieldConfig> &vLogFieldConfig, int32_t &iFieldIndex)
{
	TiXmlElement *groups = root->FirstChildElement("groups");
	if (groups == NULL)
	{
		ERROR_RET("error: miss root/groups");
	}

	string sGroupId = node->Attribute("group") ? : "";
	TiXmlElement *pImportGroup = NULL;
	for (TiXmlElement *group = groups->FirstChildElement("group"); group != NULL; group = group->NextSiblingElement("group"))
	{
		if (group->Attribute("id") == sGroupId)
		{
			pImportGroup = group;
			break;
		}
	}

	if (pImportGroup == NULL)
	{
		ERROR_RET("error: invalid group id: " << sGroupId);
	}

	string sParam = node->Attribute("param") ? : "";
	map<string, string> mVariable;
	UtilString::splitString2(sParam, "&", "=", mVariable);

	for (TiXmlElement *field = pImportGroup->FirstChildElement("field"); field != NULL; field = field ->NextSiblingElement("field"))
	{
		LogFieldConfig stLogFieldConfig;
		if (parseLogFieldConfig(field, stLogFieldConfig, iFieldIndex) != 0)
		{
			return -1;
		}

		stLogFieldConfig.sMatch = UtilString::replaceVariable(stLogFieldConfig.sMatch, mVariable, UtilString::REPLACE_MODE_ALL_MATCH);
		vLogFieldConfig.push_back(stLogFieldConfig);
	}
	
	return 0;
}

int32_t CSyncLog::loadSkelConfig(const string &sConfigFile)
{
	TiXmlDocument doc;
	if (!doc.LoadFile(sConfigFile))
	{
		ERROR_RET("cannot open xml config file: " << sConfigFile << "," << doc.ErrorDesc());
	}

	TiXmlElement *root = doc.RootElement();
	if (root == NULL)
	{
		ERROR_RET("error: miss root");
	}

	for (TiXmlElement *file = root->FirstChildElement("file"); file != NULL; file = file->NextSiblingElement("file"))
	{
		LogFileConfig stLogFileConfig;
		stLogFileConfig.sLogFileName = replaceVariable(file->Attribute("name") ? : "");
		if (stLogFileConfig.sLogFileName.empty())
		{
			ERROR_RET("error: miss root/file@name");
		}

		string sLogFileType = file->Attribute("type") ? : "";
		if (sLogFileType == "hourlog")
		{
			stLogFileConfig.emLogFileType = LogFileType_Hour;
		}
		else if (sLogFileType == "daylog")
		{
			stLogFileConfig.emLogFileType = LogFileType_Day;
		}
		else
		{
			ERROR_RET("error: invalid root/file@type, " << stLogFileConfig.sLogFileName);
		}

		uint32_t iGlobMatchNum = 0;
		for (TiXmlElement *log = file->FirstChildElement("log"); log != NULL; log = log->NextSiblingElement("log"))
		{
			LogTableConfig stLogTableConfig;
			stLogTableConfig.sName = log->Attribute("name") ? : "";
			stLogTableConfig.sDesc = log->Attribute("desc") ? : "";

			int32_t iFieldIndex = -1;
			for (TiXmlElement *node = log->FirstChildElement(); node != NULL; node = node->NextSiblingElement())
			{
				vector<LogFieldConfig> vLogFieldConfig;
				if (node->Value() == string("field"))
				{
					LogFieldConfig stLogFieldConfig;
					if (parseLogFieldConfig(node, stLogFieldConfig, iFieldIndex) != 0)
					{
						ERROR_RET("error: invalid root/file/log/field: " << stLogFileConfig.sLogFileName << "," << stLogTableConfig.sName << "," << *node);
					}

					vLogFieldConfig.push_back(stLogFieldConfig);
				}
				else if (node->Value() == string("import"))
				{
					if (parseImportFieldConfig(root, node, vLogFieldConfig, iFieldIndex) != 0)
					{
						ERROR_RET("error: invalid root/file/log/import: " << stLogFileConfig.sLogFileName << "," << stLogTableConfig.sName << "," << *node);
					}
				}
				else
				{
					ERROR_RET("error: invalid root/file/log child: " << stLogFileConfig.sLogFileName << "," << stLogTableConfig.sName << "," << node->Value());
				}

				for (uint32_t j = 0; j < vLogFieldConfig.size(); ++j)
				{
					const LogFieldConfig &stLogFieldConfig = vLogFieldConfig[j];
					stLogTableConfig.vFieldConfig.push_back(stLogFieldConfig);
					if (!stLogFieldConfig.sMatch.empty())
					{
						stLogTableConfig.vMatchFieldIndex.push_back(stLogFieldConfig.iFieldIndex);
					}
				}
			}

			if (stLogTableConfig.vFieldConfig.empty())
			{
				ERROR_RET("error: no field, " << stLogFileConfig.sLogFileName << "," << stLogTableConfig.sName);
			}

			stLogFileConfig.vLogTableConfig.push_back(stLogTableConfig);
			if (stLogTableConfig.vMatchFieldIndex.empty())
			{
				++iGlobMatchNum;
			}
		}

		if (iGlobMatchNum > 1)
		{
			ERROR_RET("error: glob match num should be <= 1, " << stLogFileConfig.sLogFileName);
		}
		m_stSyncConfig.vLogFileConfig.push_back(stLogFileConfig);
	}

	return 0;
}

int32_t CSyncLog::buildTableConfigFinder()
{
	for (unsigned i = 0; i < m_stSyncConfig.vLogFileConfig.size(); ++i)
	{
		const LogFileConfig &stLogFileConfig = m_stSyncConfig.vLogFileConfig[i];
		CTableConfigFinder &finder = m_mTableConfigFinder[stLogFileConfig.sLogFileName];
		finder.init(stLogFileConfig.vLogTableConfig);
	}
	return 0;
}

int32_t CSyncLog::prepareDatabase(bool bAutoCreateDb)
{
	m_mysql.init(m_stSyncConfig.stDBConf);

	if (bAutoCreateDb)
	{
		try
		{
			ostringstream os;
			os << "CREATE DATABASE IF NOT EXISTS " << m_stSyncConfig.stDBConf.sDatabase;

			CDBConf stConf = m_stSyncConfig.stDBConf;
			stConf.sDatabase = "";
			CMysql mysql(stConf);
			mysql.execute(os.str());
		}
		catch (std::exception &e)
		{
			LOG_ERROR("create database: " << e.what());
			return -1;
		}
	}

	set<string> setTable;
	int32_t ret = getAllTableInDB(setTable);
	if (ret != 0)
	{
		return ret;
	}

	for (unsigned i = 0; i < m_stSyncConfig.vLogFileConfig.size(); ++i)
	{
		const LogFileConfig &stLogFileConfig = m_stSyncConfig.vLogFileConfig[i];
		for (unsigned i = 0; i < stLogFileConfig.vLogTableConfig.size(); ++i)
		{
			const LogTableConfig &stLogTableConfig = stLogFileConfig.vLogTableConfig[i];
			if (setTable.find(stLogTableConfig.sName) == setTable.end())
			{
				ret = createNewTable(stLogTableConfig);
			}
			else
			{
				ret = checkAndAlterTable(stLogTableConfig);
			}
			if (ret != 0)
			{
			    LOG_ERROR("create/alter table: " << stLogTableConfig.sName << " failed");
				return ret;
			}
		}
	}
	return 0;
}

int32_t CSyncLog::getAllTableInDB(set<string> &setTable)
{
	try
	{
		CMysqlResultPtr res = m_mysql.query("SHOW TABLES");
		while (res->next())
		{
			setTable.insert(res->get(0));
		}
		return 0;
	}
	catch (std::exception &e)
	{
		LOG_ERROR("list table: " << e.what());
	}
	return -1;
}

int32_t CSyncLog::createNewTable(const LogTableConfig &stLogTableConfig)
{
	try
	{
		ostringstream os;
		os << "CREATE TABLE IF NOT EXISTS " << stLogTableConfig.sName << " (";
		os << "_rid INT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY ";
		for (unsigned i = 0; i < stLogTableConfig.vFieldConfig.size(); ++i)
		{
			const LogFieldConfig &stLogFieldConfig = stLogTableConfig.vFieldConfig[i];
			string result = stLogFieldConfig.getCreateFields();
			if (result.empty()) continue;

			os << ",";
			os << result;
		}
		os << ") DEFAULT CHARSET=UTF8;";

		LOG_DEBUG(os.str());
		m_mysql.execute(os.str());
		return 0;
	}
	catch (std::exception &e)
	{
		LOG_ERROR("create table: " << stLogTableConfig.sName << "," << e.what());
	}
	return -1;
}

int32_t CSyncLog::checkAndAlterTable(const LogTableConfig &stLogTableConfig)
{
	try
	{
		ostringstream os;
		os << "DESC " << stLogTableConfig.sName;

		set<string> setCurColumn;
		CMysqlResultPtr res = m_mysql.query(os.str());
		while (res->next())
		{
			setCurColumn.insert(res->get(0));
		}

		os.str("");
		os << "ALTER TABLE " << stLogTableConfig.sName << " ";
		int iAddCount = 0;
		vector<LogFieldConfig> vDBColumn;
		stLogTableConfig.getAllDBColumns(vDBColumn);
		for (int i = vDBColumn.size() - 1; i >= 0; --i)
		{
			const LogFieldConfig &stLogFieldConfig = vDBColumn[i];
			if (setCurColumn.find(stLogFieldConfig.sColumn) != setCurColumn.end())
			{
				continue;
			}

			if (iAddCount != 0)
			{
				os << ",";
			}
			os << "ADD " << stLogFieldConfig.sColumn << " " << getSqlFieldType(stLogFieldConfig.iFieldDataType);

			bool bAfter = false;
			int j = i - 1;
			for (; j >= 0; --j)
			{
				const LogFieldConfig &stLogFieldConfigPre = vDBColumn[j];
				if (setCurColumn.find(stLogFieldConfigPre.sColumn) != setCurColumn.end())
				{
					bAfter = true;
					break;
				}
			}
			if (!bAfter)
			{
				os << " FIRST";
			}
			else
			{
				const LogFieldConfig &stLogFieldConfigPre = vDBColumn[j];
				os << " AFTER " << stLogFieldConfigPre.sColumn;
			}
			++iAddCount;
		}

		if (iAddCount != 0)
		{
			LOG_DEBUG("change table: " << os.str());
			m_mysql.execute(os.str());
		}
		return 0;
	}
	catch (std::exception &e)
	{
		LOG_ERROR("alter table: " << stLogTableConfig.sName << "," << e.what());
	}
	return -1;
}

int32_t CSyncLog::prepareFileLoader()
{
	int32_t ret = m_progressManager.init(m_stSyncConfig.sProgressFile);
	if (ret != 0)
	{
		return ret;
	}

	for (unsigned i = 0; i < m_stSyncConfig.vLogFileConfig.size(); ++i)
	{
		const LogFileConfig &stLogFileConfig = m_stSyncConfig.vLogFileConfig[i];
		if (m_mFileLoaderPtr.find(stLogFileConfig.sLogFileName) != m_mFileLoaderPtr.end())
		{
			continue;
		}

		CFileLoaderPtr loaderptr(new CFileLoader());
		m_mFileLoaderPtr[stLogFileConfig.sLogFileName] = loaderptr;
		ProgressInfo stProgressInfo = m_progressManager.getProgress(stLogFileConfig.sLogFileName);
		ret = loaderptr->init(m_stSyncConfig.sLogPath, stLogFileConfig.sLogFileName, stLogFileConfig.emLogFileType, stProgressInfo);
		if (ret != 0)
		{
			return ret;
		}
	}
	return 0;
}

int32_t CSyncLog::processLog()
{
	while (true)
	{
		if (g_bQuitApplicaiton)
		{
			return -2;
		}

		uint32_t iBeginTime = UtilTime::getNow();
		for (map<string, CFileLoaderPtr>::iterator first = m_mFileLoaderPtr.begin(), last = m_mFileLoaderPtr.end(); first != last; ++first)
		{
			CFileLoader &loader = *first->second;

			int32_t ret = syncOneFile(loader);
			if (ret != 0)
			{
				return ret;
			}
		}

		uint32_t iNow = UtilTime::getNow();
		uint32_t iSleep = UtilArith::safeSub(iBeginTime + m_stSyncConfig.iSyncInterval, iNow);
		if (iSleep > 0)
		{
			LOG_DEBUG("enter sleep:" << iSleep);
			sleep(iSleep);
		}
	}
	return 0;
}

int32_t CSyncLog::syncOneFile(CFileLoader &loader)
{
	CTableConfigFinder &finder = m_mTableConfigFinder[loader.getLogFileName()];
	while (true)
	{
		uint32_t iLine = 0, iCount = 0;
		CSqlBuilder builder(m_mysql);
		ProgressInfo stOldProgressInfo = loader.getProgress();
		for (; iLine < m_stSyncConfig.iLinesPerLoop; ++iLine)
		{
			vector<string> vField;
			int32_t ret = loader.getLine(vField);
			if (ret != 0)
			{
				return ret;
			}
			if (vField.empty())
			{
				break;
			}

			const LogTableConfig *pstLogTableConfig = finder.findConfig(vField);
			if (pstLogTableConfig == NULL)
			{
				continue;
			}

			// LOG_DEBUG(m_sLogFileName << " match table: " << pstLogTableConfig->sName << ", " << UtilString::joinString(vField, "|"));
			try
			{
				builder.addLine(*pstLogTableConfig, vField);
			}
			catch (std::exception &e)
			{
				// mysql escape问题，下次再尝试
				LOG_ERROR("fail to escape string: " << e.what());
				loader.resetProgress(stOldProgressInfo);
				return 0;
			}
			++iCount;
		}

		for (map<string, ostringstreamptr>::iterator first = builder.m_mTableSql.begin(), last = builder.m_mTableSql.end(); first != last; ++first)
		{
			const string &sTableName = first->first;
			ostringstream &os = *first->second;

			try
			{
				m_mysql.execute(os.str());
				// LOG_DEBUG(os.str());
			}
			catch (std::exception &e)
			{
				LOG_ERROR("fail to sync db: " << sTableName << ", " << e.what());
				if (first == builder.m_mTableSql.begin())
				{
					// 第一条SQL就出错，不要保存进度，下次再次尝试
					loader.resetProgress(stOldProgressInfo);
					return 0;
				}
				else
				{
					// 第二条SQL后才出错，已经写入部分数据，所以保存本次进度，部分数据丢失
					LOG_ERROR("may lost data: " << sTableName << ", " << os.str());
				}
			}
		}

		if (iLine != 0)
		{
			m_progressManager.saveProgress(loader.getProgress());
		}

		if (g_bQuitApplicaiton)
		{
			return -2;
		}

		if (iLine < m_stSyncConfig.iLinesPerLoop)
		{
			break;
		}
	}
	return 0;
}

int32_t CSyncLog::printSkel(const string &sSkelConfig, const string &sPrintFile)
{
	m_bDisableReplaceVariable = true;
	int32_t ret = loadSkelConfig(sSkelConfig);
	if (ret != 0)
	{
		return ret;
	}

	ostringstream os;
	int n = 0;
	for (unsigned i = 0; i < m_stSyncConfig.vLogFileConfig.size(); ++i)
	{
		const LogFileConfig &stLogFileConfig = m_stSyncConfig.vLogFileConfig[i];
		for (unsigned i = 0; i < stLogFileConfig.vLogTableConfig.size(); ++i)
		{
			const LogTableConfig &stLogTableConfig = stLogFileConfig.vLogTableConfig[i];
			if (n++ != 0) os << endl << endl;
			os << stLogTableConfig.sName << ": " << stLogTableConfig.sDesc << endl;

			vector<LogFieldConfig> vDBColumn;
			stLogTableConfig.getAllDBColumns(vDBColumn);
			for (unsigned i = 0; i < vDBColumn.size(); ++i)
			{
				const LogFieldConfig &stLogFieldConfig = vDBColumn[i];
				os << "  " << stLogFieldConfig.sColumn << ": " << stLogFieldConfig.sDesc << endl;
			}
		}
	}

	string sText = os.str();
	if (sPrintFile.empty())
	{
		cout << sText;
	}
	else
	{
		UtilFile::saveToFile(sPrintFile, sText);
	}
	return 0;
}

string CSyncLog::replaceVariable(const string &sData)
{
	if (m_bDisableReplaceVariable)
	{
		return sData;
	}
	return UtilString::replaceVariable(sData, m_stSyncConfig.mVariable, UtilString::REPLACE_MODE_ALL_MATCH);
}
