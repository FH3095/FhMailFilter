#pragma once

#include "../Util.h"
#include "Db.h"
#include <INIReader.h>
#include <memory>
#include <string>
#include <set>
#include <list>
#include <regex>
#include <cstdint>

class Config {
private:
	LOG_LOGGERVAR();
	static std::unique_ptr<Config> instance;
	static const std::regex_constants::syntax_option_type regexFlags;
	std::string listenAddress;
	int milterLogLevel;
	int userId;
	int groupId;
	unsigned int socketUMask;
	std::set<std::string> postmasterAddresses;
	std::string postmasterAddressHashPrefix;
	std::int32_t dnsBlCacheTime;
	std::int32_t dnsBlLookupTimeout;
	std::string dnsBlWarnHeaderName;
	std::unique_ptr<Db> db;
	std::list<std::string> localAndRelayDomains;
	std::list<std::regex> localPartForLocalPatterns;
	std::list<std::regex> localPartForRemotePatterns;
	//private static final int PATTERN_FLAGS = Pattern.CASE_INSENSITIVE | Pattern.DOTALL | Pattern.UNICODE_CASE;
	std::string getConfigStringValue(INIReader& reader, const std::string& section,const std::string& name);
	std::int32_t getConfigIntValue(INIReader& reader,const std::string& section,const std::string& name,const int base = 10);
	Config(const char* const configFile);

public:
	virtual ~Config();
	static Config& getInstance();
	static void readConfig();

	inline Db& getDb()
	{	return *db;}

	inline const std::string& getListenAddress() const
	{	return listenAddress;}

	inline int getMilterLogLevel() const
	{	return milterLogLevel;}

	inline int getUserId() const
	{	return userId;}

	inline int getGroupId() const
	{	return groupId;}

	inline int getSocketUMask() const
	{	return socketUMask;}

	inline const std::set<std::string>& getPostmasterAddresses() const
	{	return postmasterAddresses;}

	inline const std::string& getPostmasterAddressHashPrefix() const
	{	return postmasterAddressHashPrefix;}

	inline std::int32_t getDnsBlCacheTime() const
	{	return dnsBlCacheTime;}

	inline std::int32_t getDnsBlLookupTimeout() const
	{	return dnsBlLookupTimeout;}

	inline const std::string& getDnsBlWarnHeaderName() const
	{	return dnsBlWarnHeaderName;}

	inline const std::list<std::string>& getLocalAndRelayDomains() const
	{	return localAndRelayDomains;}

	inline const std::list<std::regex>& getLocalPartForLocalPatterns() const
	{	return localPartForLocalPatterns;}

	inline const std::list<std::regex>& getLocalPartForRemotePatterns() const
	{	return localPartForRemotePatterns;}
};
