#pragma once

#include "../MilterHandler.h"
#include "../Milter.h"
#include "../../log.h"

class CheckUnautenticatedSenderRcpt: public MilterHandler {
private:
	LOG_LOGGERVAR();
	Milter& milter;
public:
	CheckUnautenticatedSenderRcpt(Milter& milter);
	virtual ~CheckUnautenticatedSenderRcpt();

	virtual MilterResult envfrom() {
		return MILTER_OPERATION_NO_RESULT;
	}

	virtual MilterResult envrcpt(const std::string& rcpt) {
		return MILTER_OPERATION_NO_RESULT;
	}

	inline static unsigned long getSupportedProcesses() {
		return MilterHandler::FROM | MilterHandler::RCPT;
	}
	inline static unsigned long getRequiredModifications() {
		return 0;
	}
};
