#ifndef _MFW_UTIL_MYSQL_
#define _MFW_UTIL_MYSQL_

#include <map>
#include <vector>
#include <string>
#include <tr1/memory>
#include "util_string.h"
#include "util_noncopyable.h"
#include "mysql.h"
using namespace std;

namespace mfw
{

class CDBConf
{
public:
	string		sHost;
	uint16_t	iPort;
	string		sDatabase;
	string		sUser;
	string		sPassword;
	string		sCharset;

	CDBConf() : iPort(3306) {}

	void set(const map<string, string> &m);
};

class CMysqlResult : public noncopyable
{
public:
	explicit CMysqlResult(MYSQL_RES *res);
	~CMysqlResult();

	bool next();
	string operator[](uint32_t iIndex);
	string operator[](const string &sField);
	string get(uint32_t iIndex);
	string get(const string &sField);
	template <typename T> T get(uint32_t iIndex) { return UtilString::strto<T>(get(iIndex)); }
	template <typename T> T get(const string &sField) { return UtilString::strto<T>(get(sField)); }

	void reset();
	uint32_t count() { return m_iRowNum; }

private:
	MYSQL_RES *m_res;
	char **m_row;
	unsigned long *m_rowlength;
	map<string, uint32_t> m_mField;
	uint32_t m_iRowNum;
};

typedef tr1::shared_ptr<CMysqlResult> CMysqlResultPtr;

class CMysql : public noncopyable
{
public:
	CMysql() : m_mysql(NULL) {}
	CMysql(const CDBConf &dbconf) : m_dbconf(dbconf), m_mysql(NULL) {}
	~CMysql();

	MYSQL *getMysqlHandle() { return m_mysql; }

	void init(const CDBConf &dbconf);
	void connect();
	void disconnect();
	string escape(const string &s);
	void execute(const string &sql);
    uint64_t getInsertId();
    uint64_t getAffectedRows();
	CMysqlResultPtr query(const string &sql);

private:
    void execute(const string &sql, bool freeresult);

private:
	CDBConf		m_dbconf;
	MYSQL		*m_mysql;
};

class CMysqlTransaction : public noncopyable
{
public:
    CMysqlTransaction(CMysql &mysql, bool bRollbackOnFinish = true);
    ~CMysqlTransaction();

    void commit();
    void rollback();
    void release();

private:
    CMysql &m_mysql;
    bool m_bReleased;
    bool m_bRollbackOnFinish;
};

}

#endif
