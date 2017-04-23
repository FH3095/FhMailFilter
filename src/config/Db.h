#pragma once

#include <memory>
#include <map>
#include <string>
#include <mutex>
#include <mysql_driver.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>
#include "../Util.h"

class Db {
private:
	LOG_LOGGERVAR();
	typedef std::map<std::string, std::unique_ptr<sql::PreparedStatement>> StatementCacheType;
	sql::mysql::MySQL_Driver* driver;
	std::unique_ptr<sql::Connection> connection;
	std::recursive_mutex mutex;
	std::string url;
	std::string username;
	std::string password;
	StatementCacheType preparedStatementCache;
	void tryReconnect(const std::exception* const exception);
public:
	typedef std::unique_ptr<sql::ResultSet> ResultSetType;
	typedef sql::PreparedStatement& StmtType;
	Db(const std::string url, const std::string& user,
			const std::string& password);
	virtual ~Db();
	StmtType getStatement(const std::string& sql);
};
