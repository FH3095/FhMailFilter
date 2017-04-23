#pragma once

#include "../MilterHandler.h"
#include "../Milter.h"
#include "../../log.h"
#include <regex>
#include <string>

class CheckLocalPart: public MilterHandler {
private:
	LOG_LOGGERVAR();
	static const std::string STATUS_REJECT_CODE;
	static const std::string STATUS_REJECT_EXTENDED_CODE;
	Milter& milter;
public:
	CheckLocalPart(Milter& milter);
	virtual ~CheckLocalPart();

	virtual MilterResult envrcpt(const std::string& rcpt);

	inline static unsigned long getSupportedProcesses() {
		return MilterHandler::RCPT;
	}
	inline static unsigned long getRequiredModifications() {
		return 0;
	}
};
