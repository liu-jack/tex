#include "util_mysql.h"
#include "util_string.h"
#include "util_log.h"
#include "errmsg.h"
#include <stdexcept>
#include <cstring>
using namespace std;

namespace mfw
{

void CDBConf::set(const map<string, string> &m)
{
	map<string, string>::const_iterator it;
	if ((it = m.find("dbhost")) != m.end()) sHost = it->second;
	if ((it = m.find("dbport")) != m.end()) iPort = UtilString::strto<uint16_t>(it->second);
	if ((it = m.find("dbname")) != m.end()) sDatabase = it->second;
	if ((it = m.find("dbuser")) != m.end()) sUser = it->second;
	if ((it = m.find("dbpass")) != m.end()) sPassword = it->second;
	if ((it = m.find("charset")) != m.end()) sCharset = it->second;
}

CMysqlResult::CMysqlResult(MYSQL_RES *res)
	: m_res(res), m_row(NULL),m_rowlength(NULL), m_iRowNum(0)
{
	if (m_res == NULL)
	{
		return;
	}

	uint32_t n = mysql_num_fields(m_res);
	MYSQL_FIELD *fields = mysql_fetch_fields(m_res);
	for (uint32_t i = 0; i < n; ++i)
	{
		m_mField[fields[i].name] = i;
	}
	m_iRowNum = mysql_num_rows(m_res);
}

CMysqlResult::~CMysqlResult()
{
	if (m_res)
	{
		mysql_free_result(m_res);
	}
}

bool CMysqlResult::next()
{
	if (m_res)
	{
		m_row = mysql_fetch_row(m_res);
		if (m_row)
		{
			m_rowlength = mysql_fetch_lengths(m_res);
			return true;
		}
		else
		{
			m_rowlength = NULL;
			return false;
		}
	}
	return false;
}

string CMysqlResult::operator[](uint32_t iIndex)
{
	return get(iIndex);
}

string CMysqlResult::operator[](const string &sField)
{
	return get(sField);
}

string CMysqlResult::get(uint32_t iIndex)
{
	if (m_res == NULL)
	{
		throw std::runtime_error("empty mysql result set");
	}
	if (iIndex >= m_mField.size())
	{
		throw std::runtime_error("exceed mysql result set field");
	}
	if (m_row == NULL)
	{
		throw std::runtime_error("invalid mysql result set row");
	}
	if (m_row[iIndex] == NULL || m_rowlength[iIndex] == 0)
	{
		return "";
	}
	return string(m_row[iIndex], m_rowlength[iIndex]);
}

string CMysqlResult::get(const string &sField)
{
	map<string, uint32_t>::const_iterator it = m_mField.find(sField);
	if (it == m_mField.end())
	{
		throw std::runtime_error("mysql result set field doesn't exist: " + sField);
	}
	return (*this)[it->second];
}

void CMysqlResult::reset()
{
	if (m_res)
	{
		mysql_data_seek(m_res, 0);
		m_row = NULL;
		m_rowlength = NULL;
	}
}

CMysql::~CMysql()
{
	disconnect();
}

void CMysql::init(const CDBConf &dbconf)
{
	disconnect();
	m_dbconf = dbconf;
}

void CMysql::connect()
{
	if (m_mysql != NULL)
	{
		return;
	}

	m_mysql = mysql_init(NULL);
	if (m_mysql == NULL)
	{
		throw std::runtime_error("mysql_init: " + string(mysql_error(m_mysql)));
	}

    if(!m_dbconf.sCharset.empty())
	{
    	int ret = mysql_options(m_mysql, MYSQL_SET_CHARSET_NAME, m_dbconf.sCharset.c_str());
		if (ret != 0)
		{
			string sError = mysql_error(m_mysql);
			mysql_close(m_mysql);
			m_mysql = NULL;
			throw std::runtime_error("mysql_options MYSQL_SET_CHARSET_NAME " + m_dbconf.sCharset + ": " + sError);
		}
	}

	char value = 0;
	mysql_options(m_mysql, MYSQL_OPT_RECONNECT, &value);

    if (mysql_real_connect(m_mysql, m_dbconf.sHost.c_str(), m_dbconf.sUser.c_str(), m_dbconf.sPassword.c_str(), m_dbconf.sDatabase.c_str(), m_dbconf.iPort, NULL, CLIENT_MULTI_STATEMENTS) == NULL)
    {
    	string sError = mysql_error(m_mysql);
    	mysql_close(m_mysql);
		m_mysql = NULL;
    	throw std::runtime_error("mysql_real_connect: " + sError);
    }
}

void CMysql::disconnect()
{
	if (m_mysql == NULL)
	{
		return;
	}
	mysql_close(m_mysql);
	m_mysql = NULL;
}

string CMysql::escape(const string &s)
{
	connect();

	string r;
	r.resize(s.size() * 2 + 1);
	mysql_real_escape_string(m_mysql, &r[0], s.c_str(), s.size());
	r.resize(strlen(r.c_str()));
	return r;
}

void CMysql::execute(const string &sql)
{
    execute(sql, true);
}

void CMysql::execute(const string &sql, bool freeresult)
{
	connect();

	int ret = mysql_real_query(m_mysql, sql.c_str(), sql.length());
	if (ret == 0)
	{
        if (freeresult)
        {
            do  
            {  
                MYSQL_RES *res = mysql_store_result(m_mysql);  
                mysql_free_result(res); 
            } while(!mysql_next_result(m_mysql)); 
        }
		return;
	}

	ret = mysql_errno(m_mysql);
	if (ret == CR_SERVER_GONE_ERROR || ret == CR_SERVER_LOST)
	{
		ret = mysql_real_query(m_mysql, sql.c_str(), sql.length());
		if (ret == 0)
		{
            if (freeresult)
            {
                do  
                {  
                    MYSQL_RES *res = mysql_store_result(m_mysql);  
                    mysql_free_result(res); 
                } while(!mysql_next_result(m_mysql)); 
            }
			return;
		}
	}

	throw std::runtime_error("mysql_real_query: " + string(mysql_error(m_mysql)) + ", " + sql);
}

uint64_t CMysql::getInsertId()
{
    if (m_mysql == NULL)
	{
		return 0;
	}

    return mysql_insert_id(m_mysql);
}

uint64_t CMysql::getAffectedRows()
{
    if (m_mysql == NULL)
	{
		return 0;
	}

    return mysql_affected_rows(m_mysql);
}

CMysqlResultPtr CMysql::query(const string &sql)
{
	execute(sql, false);
	MYSQL_RES *res = mysql_store_result(m_mysql);
	if (res == NULL)
	{
		throw std::runtime_error("mysql_store_result: " + string(mysql_error(m_mysql)) + ", " + sql);
	}

	CMysqlResultPtr resptr(new CMysqlResult(res));
	return resptr;
}

CMysqlTransaction::CMysqlTransaction(CMysql &mysql, bool bRollbackOnFinish /*=true*/)
    : m_mysql(mysql)
    , m_bReleased(true)
    , m_bRollbackOnFinish(bRollbackOnFinish) {

    m_mysql.connect();

    if (mysql_autocommit(m_mysql.getMysqlHandle(), 0) != 0) {
        throw std::runtime_error("mysql_autocommit failed");
    }
    m_bReleased = false;
}

CMysqlTransaction::~CMysqlTransaction() {
    if (m_bReleased) return;

    if (m_bRollbackOnFinish) {
        rollback();
    }

    release();
}

void CMysqlTransaction::commit() {

    m_mysql.connect();

    if (mysql_commit(m_mysql.getMysqlHandle()) != 0) {
        throw std::runtime_error("mysql_commit failed");
    }
}

void CMysqlTransaction::rollback() {

    m_mysql.connect();

    if (mysql_rollback(m_mysql.getMysqlHandle()) != 0) {
        throw std::runtime_error("mysql_rollback failed");
    }
}

void CMysqlTransaction::release() {
    if (m_bReleased) return;

    m_mysql.connect();

    if (mysql_autocommit(m_mysql.getMysqlHandle(), 1) != 0) {
        throw std::runtime_error("mysql_autocommit failed");
    }
    m_bReleased = true;
}

}
