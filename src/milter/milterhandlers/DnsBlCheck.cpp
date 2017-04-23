#define _POSIX_C_SOURCE 200112L
#include "DnsBlCheck.h"
#include "../../config/Config.h"
#include "../../config/Db.h"
#include <netinet/in.h>
#include <resolv.h>
#include <arpa/nameser.h>
#include <netdb.h>
#include <string.h>

LOG_LOGGERINIT(DnsBlCheck, "milter.milterhandlers.DnsBlCheck");
const std::string DnsBlCheck::REQUESTED_MACROS("{auth_authen}");
const std::string DnsBlCheck::SQL_QUERY(
		"SELECT ad.dnsbl_list AS dnsbl_list, ad.dnsbl_reject AS dnsbl_reject "
				"FROM addresses ad INNER JOIN domains d ON d.id = ad.domain_id "
				"WHERE ad.active = 1 AND ad.address = ? AND d.domain = ? "
				"ORDER BY ad.id");
const std::string DnsBlCheck::STATUS_REJECT_CODE("550");
const std::string DnsBlCheck::STATUS_REJECT_EXTENDED_CODE("5.7.1");
const std::string DnsBlCheck::MILTER_MACRO_LOGIN_NAME("{auth_authen}");
std::recursive_mutex DnsBlCheck::blResultMutex;
std::recursive_mutex DnsBlCheck::sqlQueryMutex;
DnsBlCheck::BlResultCacheMap DnsBlCheck::blResultCache;

DnsBlCheck::DnsBlCheck(Milter& milter) :
		milter(milter), userIsLoggedIn(false) {
}

DnsBlCheck::~DnsBlCheck() {
}

std::string DnsBlCheck::buildDnsName(const std::string& dnsBl, const _SOCK_ADDR* genericAddress)
{
	if(genericAddress->sa_family!=AF_INET)
	{
		LOG_DEBUG() << "Tried to build DnsName for dnsBl " << dnsBl << " but sockaddr is not an ipv4-address but instead " << genericAddress->sa_family;
		return std::string();
	}
	std::stringstream ret;
	ret.str().reserve(64);
	const sockaddr_in* address=(const sockaddr_in*)genericAddress;
	const unsigned char* addrPtr=(const unsigned char*)(&(address->sin_addr.s_addr));
	for(signed short i=3;i>=0;--i)
	{
		ret << ((int)addrPtr[i]) << ".";
	}
	ret << dnsBl;
	return ret.str();
}

void DnsBlCheck::addDnsBlCacheEntry(const std::string& dnsBlName,
		const std::string& dnsBlResult) {
	MutexLock lock(blResultMutex);
	blResultCache.emplace(dnsBlName,
			BlResultCachePair(
					std::chrono::steady_clock::now()
							+ std::chrono::seconds(
									Config::getInstance().getDnsBlCacheTime()),
					dnsBlResult));
}

void DnsBlCheck::printNsError(const char* msg) {
	const int errorNumber = h_errno;
	std::stringstream out;
	out << "HostError: " << msg << ": " << errorNumber << " (";
	switch (errorNumber) {
	case HOST_NOT_FOUND:
		out << "Unknown Zone";
		break;
	case NO_DATA:
		out << "No NS records";
		break;
	case TRY_AGAIN:
		out << "No response for query";
		break;
	default:
		out << "Unexpected error";
		break;
	}
	out << ")";
	LOG_ERROR() << (out.str());
}

void DnsBlCheck::printNsReturnCodeError(const int errorNumber,
		const char* msg) {
	std::stringstream out;
	out << "DNS ReturnError: " << msg << ": " << errorNumber << " (";
	switch (errorNumber) {
	case ns_r_formerr:
		out << "FORMERR";
		break;
	case ns_r_servfail:
		out << "SERVFAIL";
		break;
	case ns_r_nxdomain:
		out << "NXDOMAIN";
		break;
	case ns_r_notimpl:
		out << "NOTIMP";
		break;
	case ns_r_refused:
		out << "REFUSED";
		break;
	default:
		out << "Unexpected return code";
		break;
	}
	out << ")";
	LOG_ERROR() << (out.str());
}

void DnsBlCheck::printLibError(const char* msg) {
	const int errorNumber = errno;
	char errorBuf[512];
	if (strerror_r(errorNumber, errorBuf, sizeof(errorBuf)) != 0) {
		// Error while trying to copy error-message to errorBuf
		errorBuf[0] = '\0';
	}
	LOG_ERROR() << "LibError: " << msg << ": " << errorNumber << " ("
			<< errorBuf << ")";
}

std::string DnsBlCheck::queryDnsBl(const std::string& dnsBlName) {
	MutexLock blLock(blResultMutex);
	BlResultCacheMap::iterator cacheResultIt = blResultCache.find(dnsBlName);
	if (blResultCache.end() != cacheResultIt
			&& cacheResultIt->second.first
					>= std::chrono::steady_clock::now()) {
		return cacheResultIt->second.second;
	}
	blLock.release();
	// OK, no cached result. Query nameserver C-Style
	static const unsigned int NS_BUFFER_SIZE = NS_MAXMSG;
	std::unique_ptr<unsigned char[]> nsBuf(new unsigned char[NS_BUFFER_SIZE]);
	int result, msgCount;
	ns_msg msg;
	ns_rr rr;
	std::list<std::shared_ptr<uint8_t>> ips;
	std::list<std::string> txts;

	// Clear errno
	errno = 0;
	// A records first
	result = res_query(dnsBlName.c_str(), ns_c_in, ns_t_a, nsBuf.get(),
			NS_BUFFER_SIZE);
	if (result < 0) {
		if (errno != 0) {
			printLibError("res_query for A failed");
		} else {
			// HOST_NOT_FOUND=NXDOMAIN, which means not blacklisted in DnsBl.
			if (h_errno == HOST_NOT_FOUND) {
				addDnsBlCacheEntry(dnsBlName, "");
				return std::string();
			}
			printNsError("res_query for A failed");
		}
		return std::string();
	}
	if (ns_initparse(nsBuf.get(), result, &msg)) {
		printLibError("ns_initparse for A failed");
		return std::string();
	}
	if (ns_msg_getflag(msg, ns_f_rcode) != ns_r_noerror) {
		printNsReturnCodeError(ns_msg_getflag(msg, ns_f_rcode),
				"NS-Response for A is error");
		return std::string();
	}
	msgCount = ns_msg_count(msg, ns_s_an);
	for (int i = 0; i < msgCount; ++i) {
		if (ns_parserr(&msg, ns_s_an, i, &rr)) {
			printLibError("ns_parserr for A failed");
			return std::string();
		}
		if (ns_rr_type(rr) == ns_t_a) {
			std::shared_ptr<uint8_t> ip = sharedArray(new uint8_t[4]);
			memcpy(ip.get(), ns_rr_rdata(rr), sizeof(uint8_t) * 4);
			ips.push_back(ip);
		}
	}

	// If no A-Record is found, source-ip is not blacklisted
	if (ips.empty()) {
		// Add negative entry to chache
		addDnsBlCacheEntry(dnsBlName, "");
		return std::string();
	}

	// TXT RECORD
	result = res_query(dnsBlName.c_str(), ns_c_in, ns_t_txt, nsBuf.get(),
			NS_BUFFER_SIZE);
	if (result < 0) {
		if (errno != 0) {
			printLibError("res_query for TXT failed");
		} else {
			printNsError("res_query for TXT failed");
		}
		return std::string();
	}
	if (ns_initparse(nsBuf.get(), result, &msg)) {
		printLibError("ns_initparse for TXT failed");
		return std::string();
	}
	if (ns_msg_getflag(msg, ns_f_rcode) != ns_r_noerror) {
		printNsReturnCodeError(ns_msg_getflag(msg, ns_f_rcode),
				"NS-Response for TXT is error");
		return std::string();
	}
	msgCount = ns_msg_count(msg, ns_s_an);
	for (int i = 0; i < msgCount; ++i) {
		if (ns_parserr(&msg, ns_s_an, i, &rr)) {
			printLibError("ns_parserr for TXT failed");
			return std::string();
		}
		if (ns_rr_type(rr) == ns_t_txt) {
			std::string txt;
			// TXT record contains text length in first byte
			txt.reserve(ns_rr_rdata(rr)[0]);
			// Following bytes are the text
			for (int j = 1; j < ns_rr_rdlen(rr); ++j) {
				txt.push_back(ns_rr_rdata(rr)[j]);
			}
			txts.push_back(txt);
		}
	}

	// Convert A-Recods and TXT-Records to string
	std::stringstream resultString;
	for (std::list<std::shared_ptr<uint8_t>>::const_iterator it = ips.cbegin();
			it != ips.cend(); ++it) {
		for (signed short i = 0; i < 4; ++i) {
			resultString << ((int) ((it->get())[i]));
			if (i < 3) {
				resultString << ".";
			}
		}
		resultString << ' ';
	}
	for (std::list<std::string>::const_iterator it = txts.cbegin();
			it != txts.cend(); ++it) {
		resultString << (*it) << " ; ";
	}

	// Store positive result
	addDnsBlCacheEntry(dnsBlName, resultString.str());
	return resultString.str();
}

DnsBlCheck::DnsBlForUserResult DnsBlCheck::getDnsBlForUser(
		const std::string& username, const std::string& domain) {
	try {
		MutexLock lock(sqlQueryMutex);
		Db::StmtType stmt = Config::getInstance().getDb().getStatement(
				SQL_QUERY);
		stmt.setString(1, sql::SQLString(username));
		stmt.setString(2, sql::SQLString(domain));
		Db::ResultSetType result(stmt.executeQuery());
		if (!result->next()) {
			LOG_DEBUG() << "Cant find dnsbl setting for " << username << "@"
					<< domain;
			return DnsBlForUserResult();
		}
		return DnsBlForUserResult(
				new std::pair<std::string, bool>(
						result->getString(1).asStdString(),
						result->getInt(2) != 0));
	} catch (std::exception& e) {
		LOG_ERROR() << "Cant query database for dnsbl settings for " << username
				<< "@" << domain << " because " << LOG_EXCEPT(e);
		return DnsBlForUserResult();
	}
}

DnsBlCheck::MilterResult DnsBlCheck::envrcpt(const std::string& rcpt) {
	std::string username = getMailLocalPart(rcpt);
	std::string domain = getMailDomainPart(rcpt);
	LOG_DEBUG() << "Search for dnsbl settings for " << username << "@"
			<< domain;
	DnsBlForUserResult dnsBlSettings = getDnsBlForUser(username, domain);
	if (!dnsBlSettings) {
		return MILTER_OPERATION_NO_RESULT;
	}

	std::string dnsName = buildDnsName(dnsBlSettings->first,
			milter.getRemoteAddress());
	LOG_DEBUG() << "Found DnsBlSettings for " << username << "@" << domain
			<< ": " << dnsBlSettings->first << "," << dnsBlSettings->second
			<< ". Resulting dnsName="
			<< (dnsName.empty() ? "<EMPTY>" : dnsName);
	if (dnsName.empty()) {
		return MILTER_OPERATION_NO_RESULT;
	}

	std::string dnsBlResult = queryDnsBl(dnsName);
	LOG_INFO() << "DnsBlResult for " << username << "@" << domain << ": "
			<< (dnsBlResult.empty() ? "<EMPTY>" : dnsBlResult);
	if (dnsBlResult.empty()) {
		return MILTER_OPERATION_NO_RESULT;
	}

	if (dnsBlSettings->second) {
		std::stringstream msg;
		msg << dnsBlResult;
		return MilterHandler::constructResult(STATUS_REJECT_CODE,
				STATUS_REJECT_EXTENDED_CODE, msg);
	} else {
		toAddWarnings.push_back(dnsBlResult);
		return MILTER_OPERATION_NO_RESULT;
	}
}

DnsBlCheck::MilterResult DnsBlCheck::envfrom() {
	char macroName[] = "{auth_authen}";
	char* authName = smfi_getsymval(milter.getMilterContext(), macroName);
	if (authName != NULL && authName[0] != '\0') {
		LOG_DEBUG() << "Allow mail from " << milter.getRemoteHostname()
				<< " because sender " << milter.getMailFrom()
				<< " is logged in as " << authName;
		userIsLoggedIn = true;
	}
	return MILTER_OPERATION_NO_RESULT;
}

DnsBlCheck::MilterResult DnsBlCheck::eom() {
	if (userIsLoggedIn) {
		return MILTER_OPERATION_NO_RESULT;
	}
	std::string warnHeaderName = Config::getInstance().getDnsBlWarnHeaderName();
	std::shared_ptr<char> headerName = copyStringToArray(warnHeaderName);
	for (std::list<std::string>::const_iterator it = toAddWarnings.cbegin();
			it != toAddWarnings.cend(); ++it) {
		std::shared_ptr<char> headerValue = copyStringToArray(*it);
		smfi_addheader(milter.getMilterContext(), headerName.get(),
				headerValue.get());
	}
	return MILTER_OPERATION_NO_RESULT;
}
