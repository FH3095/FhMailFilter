#include "CheckLocalPart.h"
#include <list>
#include "../../config/Config.h"

LOG_LOGGERINIT(CheckLocalPart, "milter.milterhandlers.CheckLocalPart");
const std::string CheckLocalPart::STATUS_REJECT_CODE("550");
const std::string CheckLocalPart::STATUS_REJECT_EXTENDED_CODE("5.7.1");

CheckLocalPart::CheckLocalPart(Milter& milter) :
		milter(milter) {
}

CheckLocalPart::~CheckLocalPart() {
}

CheckLocalPart::MilterResult CheckLocalPart::envrcpt(const std::string& rcpt) {
	LOG_DEBUG() << "CheckLocalPart start";
	bool isLocalDomain = false;
	if (rcpt.npos == rcpt.find('@')) {
		isLocalDomain = true;
	} else {
		for (std::list<std::string>::const_iterator it =
				Config::getInstance().getLocalAndRelayDomains().cbegin();
				it != Config::getInstance().getLocalAndRelayDomains().cend();
				++it) {
			if (stringEndsWith(rcpt, *it)) {
				isLocalDomain = true;
				break;
			}
		}
	}

	std::string localPart = getMailLocalPart(rcpt);

	LOG_DEBUG() << "Check local part " << localPart << " of rcpt " << rcpt
			<< " as " << (isLocalDomain ? "local domain" : "remote domain");

	const std::list<std::regex>& regexList =
			isLocalDomain ?
					Config::getInstance().getLocalPartForLocalPatterns() :
					Config::getInstance().getLocalPartForRemotePatterns();
	for (std::list<std::regex>::const_iterator it = regexList.cbegin();
			it != regexList.cend(); ++it) {
		if (std::regex_search(localPart, *it)) {
			LOG_DEBUG() << "LocalPart matched regex";
			std::stringstream result;
			result.str().reserve(127);
			result << "Local-part from address " << rcpt << " is not allowed";
			return constructResult(STATUS_REJECT_CODE,
					STATUS_REJECT_EXTENDED_CODE, result);
		}
	}
	return MILTER_OPERATION_NO_RESULT;
}
