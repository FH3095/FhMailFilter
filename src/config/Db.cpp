#include "Db.h"
#include "../log.h"

LOG_LOGGERINIT(Db, "config.Db");

Db::Db(const std::string url, const std::string& user,
		const std::string& password) :
		url(url), username(user), password(password) {
	driver = sql::mysql::get_mysql_driver_instance();
	tryReconnect(NULL);
}

Db::~Db() {
}

void Db::tryReconnect(const std::exception* const exception) {
	std::lock_guard<std::recursive_mutex> lock(mutex);
	preparedStatementCache.clear();
	try {
		connection.release();
		connection.reset(driver->connect(url, username, password));
		connection->setTransactionIsolation(sql::TRANSACTION_SERIALIZABLE);
		connection->setAutoCommit(true);
	} catch (std::exception& e) {
		if (exception == NULL) {
			LOG_EMERG() << "Cant connect to database: " << LOG_EXCEPT(e);
			throw e;
		} else {
			LOG_EMERG() << "Cant connect to database: "
					<< LOG_EXCEPT_PTR(exception);
			LOG_EMERG() << "Reconnect to database failed: "
					<< LOG_EXCEPT(e);
			throw exception;
		}
	}
}

Db::StmtType Db::getStatement(const std::string& sql) {
	if (connection->isClosed()) {
		tryReconnect(NULL);
	}
	std::lock_guard<std::recursive_mutex> lock(mutex);
	StatementCacheType::iterator it = preparedStatementCache.find(sql);
	if (it != preparedStatementCache.end()) {
		return *(it->second);
	}
	sql::PreparedStatement* stmt;
	try {
		stmt = connection->prepareStatement(sql);
	} catch (std::exception& e) {
		// First try reconnect
		try {
			tryReconnect(&e);
		} catch (std::exception& e1) {
			// If we cant reconnect -> throw
			throw e1;
		}
		stmt = connection->prepareStatement(sql);
	}
	preparedStatementCache.emplace(sql,
			std::unique_ptr<sql::PreparedStatement>(stmt));
	return *preparedStatementCache.at(sql);
}
