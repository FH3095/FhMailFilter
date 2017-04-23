#include "PostmasterAddressCheck.h"
#include <locale>
#include <algorithm>
#include "../../config/Config.h"
#include <sodium.h>

LOG_LOGGERINIT(PostmasterAddressCheck,
		"milter.milterhandlers.PostmasterAddressCheck");
const int PostmasterAddressCheck::HASH_SUM_START = 28;
const int PostmasterAddressCheck::HASH_SUM_LENGTH = 8;
const std::string PostmasterAddressCheck::STATUS_REJECT_CODE("550");
const std::string PostmasterAddressCheck::STATUS_REJECT_EXTENDED_CODE("5.7.1");
const std::string PostmasterAddressCheck::SUBJECT_HEADER_NAME("subject");

PostmasterAddressCheck::PostmasterAddressCheck(Milter& milter) :
		milter(milter) {
	subjects.reserve(255);
}

PostmasterAddressCheck::~PostmasterAddressCheck() {
}

PostmasterAddressCheck::MilterResult PostmasterAddressCheck::envrcpt(
		const std::string& rcpt) {
	std::string localPart = MilterHandler::getMailLocalPart(rcpt);
	LOG_DEBUG() << "Check if localpart " << rcpt << " is Postmaster-Adress";
	for (std::set<std::string>::const_iterator it =
			Config::getInstance().getPostmasterAddresses().cbegin();
			it != Config::getInstance().getPostmasterAddresses().cend(); ++it) {
		if (stringEqualsIgnoreCase(localPart, *it)) {
			LOG_DEBUG() << "Mail from " << milter.getMailFrom() << " to "
					<< rcpt << " matches postmaster-address " << (*it);
			foundPostmasterAddresses.emplace(rcpt);
			return MILTER_OPERATION_NO_RESULT;
		} else {
			LOG_DEBUG() << "Mail from " << milter.getMailFrom()
					<< " with recipient " << rcpt
					<< " doesnt match postmaster-address " << (*it);
		}
	}
	LOG_DEBUG() << "Mail from " << milter.getMailFrom() << " with recipient "
			<< rcpt << " doesnt match any postmaster-like address, allow it";

	return MILTER_OPERATION_NO_RESULT;
}

PostmasterAddressCheck::MilterResult PostmasterAddressCheck::header(
		const std::string& headerField, const std::string& headerValue) {
	if (stringEqualsIgnoreCase(SUBJECT_HEADER_NAME, headerField)) {
		subjects.append(headerValue);
	}
	return MILTER_OPERATION_NO_RESULT;
}

PostmasterAddressCheck::MilterResult PostmasterAddressCheck::eom() {
	if (foundPostmasterAddresses.empty()) {
		return MILTER_OPERATION_NO_RESULT;
	}
	std::string expectedHash;
	expectedHash.reserve(32);
	unsigned char hash[crypto_generichash_BYTES];
	crypto_generichash_state hashState;
	crypto_generichash_init(&hashState, NULL, 0, sizeof(hash));
	crypto_generichash_update(&hashState,
			(const unsigned char*) Config::getInstance().getPostmasterAddressHashPrefix().c_str(),
			Config::getInstance().getPostmasterAddressHashPrefix().length());
	crypto_generichash_update(&hashState,
			(const unsigned char*) milter.getMailFrom().c_str(),
			milter.getMailFrom().length());
	crypto_generichash_final(&hashState, hash, sizeof(hash));
	std::string hashAsHex = convertToHex(hash, sizeof(hash));
	if (HASH_SUM_START + HASH_SUM_LENGTH > hashAsHex.length()) {
		LOG_ERROR() << "Hash length " << hashAsHex.length()
				<< " is shorter than " << (HASH_SUM_START + HASH_SUM_LENGTH);
		throw std::out_of_range(
				"Hash length is shorter than HASH_SUM_START+HASH_SUM_LENGTH");
	}
	expectedHash.append("[").append(
			hashAsHex.substr(HASH_SUM_START, HASH_SUM_LENGTH)).append("]");
	if (subjects.find(expectedHash) != subjects.npos) {
		LOG_DEBUG() << "Mail from " << milter.getMailFrom() << " to "
				<< (*foundPostmasterAddresses.cbegin()) << " matches "
				<< expectedHash << " in subjects " << subjects
				<< ". Mail allowed";
		return MILTER_OPERATION_NO_RESULT;
	} else {
		LOG_INFO() << "Rejected mail from " << milter.getMailFrom() << " ("
				<< milter.getRemoteHostname() << ") because "
				<< (*foundPostmasterAddresses.cbegin())
				<< " matches postmaster-address but hash " << expectedHash
				<< " not found.";
		std::stringstream msg;
		msg.str().reserve(127);
		msg << "To send messages from " << milter.getMailFrom() << " to "
				<< (*foundPostmasterAddresses.cbegin())
				<< " your subject must contain \"" << expectedHash << "\".";
		return constructResult(STATUS_REJECT_CODE, STATUS_REJECT_EXTENDED_CODE,
				msg);
	}
}
