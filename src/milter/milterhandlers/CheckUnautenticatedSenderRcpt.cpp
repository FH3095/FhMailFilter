#include "CheckUnautenticatedSenderRcpt.h"

LOG_LOGGERINIT(CheckUnautenticatedSenderRcpt,
		"milter.milterhandlers.CheckUnautenticatedSenderRcpt");

CheckUnautenticatedSenderRcpt::CheckUnautenticatedSenderRcpt(Milter& milter) :
		milter(milter) {
	// TODO Auto-generated constructor stub

}

CheckUnautenticatedSenderRcpt::~CheckUnautenticatedSenderRcpt() {
}

