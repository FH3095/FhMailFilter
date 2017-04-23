#pragma once

#include "../MilterHandler.h"
#include "../Milter.h"
#include "../../log.h"
#include <utility>
#include <unordered_map>
#include <mutex>
#include <list>
#include <chrono>
#include <cstdint>

class DnsBlCheck: public MilterHandler {
private:
	LOG_LOGGERVAR();
	static const std::string SQL_QUERY;
	static const std::string STATUS_REJECT_CODE;
	static const std::string STATUS_REJECT_EXTENDED_CODE;
	static const std::string MILTER_MACRO_LOGIN_NAME;
	static std::recursive_mutex blResultMutex;
	static std::recursive_mutex sqlQueryMutex;
	typedef std::unique_lock<std::recursive_mutex> MutexLock;
	typedef std::pair<std::chrono::steady_clock::time_point, std::string> BlResultCachePair;
	typedef std::unordered_map<std::string, BlResultCachePair> BlResultCacheMap;
	typedef std::shared_ptr<std::pair<std::string,bool>> DnsBlForUserResult;
	static BlResultCacheMap blResultCache;
	Milter& milter;
	std::list<std::string> toAddWarnings;
	bool userIsLoggedIn;

	static std::string buildDnsName(const std::string& dnsBl,const _SOCK_ADDR* genericAddress);
	static void addDnsBlCacheEntry(const std::string& dnsBlName, const std::string& dnsBlResult);
	static std::string queryDnsBl(const std::string& dnsBlName);
	static DnsBlForUserResult getDnsBlForUser(const std::string& username, const std::string& domain);
	static void printLibError(const char* msg);
	static void printNsError(const char* msg);
	static void printNsReturnCodeError(const int errorNumber, const char* msg);

public:
	static const std::string REQUESTED_MACROS;
	DnsBlCheck(Milter& milter);
	virtual ~DnsBlCheck();

	virtual MilterResult envfrom();
	virtual MilterResult envrcpt(const std::string& rcpt);
	virtual MilterResult eom();

	inline static unsigned long getSupportedProcesses() {
		return MilterHandler::RCPT | MilterHandler::EOM;
	}
	inline static unsigned long getRequiredModifications() {
		return SMFIF_ADDHDRS;
	}
};
