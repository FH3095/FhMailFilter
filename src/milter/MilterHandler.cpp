#include "MilterHandler.h"

LOG_LOGGERINIT(MilterHandler, "milter.MilterHandler");

const MilterHandler::MilterResult MilterHandler::MILTER_OPERATION_NO_RESULT =
		MilterResult(new Result(SMFIS_ALL_OPTS));

std::string MilterHandler::getMailLocalPart(const std::string& mail) {
	std::string::size_type atChar = mail.rfind('@');
	return (atChar == mail.npos) ? mail : mail.substr(0, atChar);
}
std::string MilterHandler::getMailDomainPart(const std::string& mail) {
	std::string::size_type atChar = mail.rfind('@');
	return (atChar == mail.npos) ? "" : mail.substr(atChar + 1);
}
