#include "Config.h"
#include <regex>
#include <list>

LOG_LOGGERINIT(Config, "config.Config");
std::unique_ptr<Config> Config::instance;
const std::regex_constants::syntax_option_type Config::regexFlags =
		std::regex_constants::extended | std::regex_constants::icase
				| std::regex_constants::optimize;

Config& Config::getInstance() {
	if (!instance) {
		throw IllegalStateError("Config: Config not yet initialized!");
	}
	return *instance;
}

void Config::readConfig() {
	try {
		instance.reset(new Config("config.ini"));
	} catch (std::exception& e) {
		LOG_EMERG() << L"Cant initialize config: " << e.what();
		throw e;
	}
}

std::string Config::getConfigStringValue(INIReader& reader,
		const std::string& section, const std::string& name) {
	static const std::string UNSET_VALUE("_NEVER_VALID_VALUE_");
	std::string ret = reader.Get(section, name, UNSET_VALUE);
	if (UNSET_VALUE.compare(ret) == 0) {
		throw IllegalStateError(
				std::string("Config: Missing config key ") + section + "."
						+ name);
	}
	return ret;
}

std::int32_t Config::getConfigIntValue(INIReader& reader,
		const std::string& section, const std::string& name, const int base) {
	std::int64_t tmp = std::stoll(getConfigStringValue(reader, section, name),
			0, base);
	if (tmp < INT32_MIN || tmp > INT32_MAX) {
		throw std::range_error(
				"Config: Value " + std::to_string(tmp) + " for " + section + "."
						+ name + " is outside of possible range "
						+ std::to_string(INT32_MIN) + "-"
						+ std::to_string(INT32_MAX));
	}
	return (std::int32_t) tmp;
}

Config::Config(const char* const configFile) {
	INIReader reader(configFile);

	listenAddress = getConfigStringValue(reader, "Basic", "ListenAddress");
	milterLogLevel = getConfigIntValue(reader, "Basic", "MilterLogLevel");
	userId = getConfigIntValue(reader, "Basic", "UserId");
	groupId = getConfigIntValue(reader, "Basic", "GroupId");
	socketUMask = getConfigIntValue(reader, "Basic", "SocketUMask", 8);

	std::list<std::string> tmp = splitString(
			getConfigStringValue(reader, "PostmasterProtect",
					"PostmasterAddresses"), ":");
	postmasterAddresses.insert(tmp.begin(), tmp.end());
	postmasterAddressHashPrefix = getConfigStringValue(reader,
			"PostmasterProtect", "PostmasterAddressHashPrefix");

	dnsBlCacheTime = getConfigIntValue(reader, "DnsBl", "DnsBlCacheTime");
	dnsBlLookupTimeout = getConfigIntValue(reader, "DnsBl",
			"DnsBlLookupTimeout");
	dnsBlWarnHeaderName = getConfigStringValue(reader, "DnsBl",
			"DnsBlWarnHeaderName");

	db.reset(
			new Db(getConfigStringValue(reader, "Db", "DbUrl"),
					getConfigStringValue(reader, "Db", "DbUsername"),
					getConfigStringValue(reader, "Db", "DbPassword")));

	localAndRelayDomains = splitString(
			getConfigStringValue(reader, "AddressPatternCheck",
					"LocalAndRelayDomains"), ":");

	tmp = splitString(
			getConfigStringValue(reader, "AddressPatternCheck",
					"LocalPartForLocalPatterns"), ":");
	for (std::list<std::string>::iterator it = tmp.begin(); it != tmp.end();
			++it) {
		localPartForLocalPatterns.push_back(std::regex(*it));
	}

	tmp = splitString(
			getConfigStringValue(reader, "AddressPatternCheck",
					"LocalPartForRemotePatterns"), ":");
	for (std::list<std::string>::iterator it = tmp.begin(); it != tmp.end();
			++it) {
		localPartForRemotePatterns.push_back(std::regex(*it));
	}
}

/* private int getConfigIntValue(final String key) {
 return Integer.valueOf(getConfigStringValue(key));
 }

 private String getConfigStringValue(final String key) {
 String value = (String) properties.get(key);
 if (value == null) {
 throw new IllegalStateException("Missing config key " + key);
 }
 return value;
 }

 private Config(final Properties properties) {
 this.properties = properties;
 int threadPoolSize = getConfigIntValue("ThreadPoolSize");
 if (threadPoolSize == 0 || threadPoolSize < -1) {
 throw new IllegalArgumentException("ThreadPoolSize must either be -1 or >= 1, but is " + threadPoolSize);
 }
 if (threadPoolSize == -1) {
 threadPool = Executors.newCachedThreadPool();
 } else {
 threadPool = Executors.newFixedThreadPool(threadPoolSize);
 }

 String listenHost = getConfigStringValue("ListenHost");
 int listenPort = getConfigIntValue("ListenPort"); // IANA reserved to aeroflight-ads
 listenAddress = new InetSocketAddress(listenHost, listenPort);

 this.postmasterAddresses = Collections.unmodifiableSet(new HashSet<String>(
 Arrays.asList(getConfigStringValue("PostmasterAddresses").split(STRING_SPLIT_PATTERN))));
 this.postmasterAddressHashPrefix = getConfigStringValue("PostmasterAddressHashPrefix");
 this.dnsBlCacheTime = TimeUnit.MINUTES.toMillis(getConfigIntValue("DnsBlCacheTime"));
 this.dnsBlLookupTimeout = getConfigIntValue("DnsBlLookupTimeout");
 this.dnsBlWarnHeaderName = getConfigStringValue("DnsBlWarnHeaderName");
 this.db = new Db("com.mysql.jdbc.Driver", getConfigStringValue("DbUrl"), getConfigStringValue("DbUsername"),
 getConfigStringValue("DbPassword"));
 this.localAndRelayDomains = Collections.unmodifiableList(
 Arrays.asList(getConfigStringValue("LocalAndRelayDomains").split(STRING_SPLIT_PATTERN)));
 List<Pattern> localPartPatterns = new LinkedList<Pattern>();
 for (String pattern : getConfigStringValue("LocalPartForLocalPatterns").split(STRING_SPLIT_PATTERN)) {
 localPartPatterns.add(Pattern.compile(pattern, PATTERN_FLAGS));
 }
 this.localPartForLocalPatterns = Collections.unmodifiableList(localPartPatterns);
 localPartPatterns = new LinkedList<Pattern>();
 for (String pattern : getConfigStringValue("LocalPartForRemotePatterns").split(STRING_SPLIT_PATTERN)) {
 localPartPatterns.add(Pattern.compile(pattern, PATTERN_FLAGS));
 }
 this.localPartForRemotePatterns = Collections.unmodifiableList(localPartPatterns);
 }*/

Config::~Config() {
}

