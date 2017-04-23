#pragma once

#include "../MilterHandler.h"
#include "../Milter.h"
#include "../../log.h"
#include <set>
#include <mutex>

class PostmasterAddressCheck: public MilterHandler {
private:
	LOG_LOGGERVAR();
	static const int HASH_SUM_START;
	static const int HASH_SUM_LENGTH;
	static const std::string STATUS_REJECT_CODE;
	static const std::string STATUS_REJECT_EXTENDED_CODE;
	static const std::string SUBJECT_HEADER_NAME;
	Milter& milter;
	std::set<std::string> foundPostmasterAddresses;
	std::string subjects;
public:
	PostmasterAddressCheck(Milter& milter);
	virtual ~PostmasterAddressCheck();

	virtual MilterResult envrcpt(const std::string& rcpt);
	virtual MilterResult header(const std::string& headerField,
			const std::string& headerValue);
	virtual MilterResult eom();

	inline static unsigned long getSupportedProcesses() {
		return MilterHandler::EOM | MilterHandler::RCPT | MilterHandler::HEADER;
	}
	inline static unsigned long getRequiredModifications() {
		return 0;
	}
};
