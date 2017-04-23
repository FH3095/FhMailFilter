#include "Milter.h"
#include <cstring>
#include <typeinfo>
#include "milterhandlers/CheckLocalPart.h"
#include "milterhandlers/CheckUnautenticatedSenderRcpt.h"
#include "milterhandlers/DnsBlCheck.h"
#include "milterhandlers/PostmasterAddressCheck.h"

LOG_LOGGERINIT(Milter, "milter.Milter");
const short Milter::PRIO_FIRST_READ = 100;
const short Milter::PRIO_FIRST_MODIFY = 200;
const short Milter::PRIO_MIDDLE = 300;
const short Milter::PRIO_LAST_MODIFY = 400;
const short Milter::PRIO_LAST_READ = 500;
const std::string Milter::REQUESTED_MACROS(DnsBlCheck::REQUESTED_MACROS);

Milter::Milter() {
	memset(&remoteAddress, 0, sizeof(remoteAddress));
	std::shared_ptr<MilterHandler> milterHandler;
	milterHandler.reset(new PostmasterAddressCheck(*this));
	addMilterHandlerToOperation(MilterHandler::RCPT, PRIO_MIDDLE,
			milterHandler);
	addMilterHandlerToOperation(MilterHandler::HEADER, PRIO_MIDDLE,
			milterHandler);
	addMilterHandlerToOperation(MilterHandler::EOM, PRIO_MIDDLE, milterHandler);
	milterHandler.reset(new DnsBlCheck(*this));
	addMilterHandlerToOperation(MilterHandler::FROM, PRIO_MIDDLE,
			milterHandler);
	addMilterHandlerToOperation(MilterHandler::RCPT, PRIO_MIDDLE,
			milterHandler);
	addMilterHandlerToOperation(MilterHandler::EOM, PRIO_MIDDLE, milterHandler);
	// Disabled this check, needs to allow local senders. Not yet relevant.
	/*milterHandler.reset(new CheckUnautenticatedSenderRcpt(*this));
	 addMilterHandlerToOperation(MilterHandler::FROM, PRIO_MIDDLE, milterHandler);
	 addMilterHandlerToOperation(MilterHandler::RCPT, PRIO_MIDDLE, milterHandler);*/
	milterHandler.reset(new CheckLocalPart(*this));
	addMilterHandlerToOperation(MilterHandler::RCPT, PRIO_MIDDLE,
			milterHandler);
	// TODO Encrypt Mail
}

Milter::~Milter() {
}

inline Milter& Milter::getMilterFromContext(SMFICTX* ctx) {
	Milter* milter = (Milter*) smfi_getpriv(ctx);
	if (milter == NULL) {
		LOG_ERROR() << "Milter context is null!";
		throw new IllegalStateError("Milter: Context is NULL");
	}
	return *milter;
}

void Milter::fillMilterStruct(smfiDesc& desc) {
	unsigned long supportedProcesses = MilterHandler::CONNECT
			| MilterHandler::FROM;
	supportedProcesses |= CheckLocalPart::getSupportedProcesses()
			| CheckUnautenticatedSenderRcpt::getSupportedProcesses()
			| DnsBlCheck::getSupportedProcesses()
			| PostmasterAddressCheck::getSupportedProcesses();
	unsigned long requiredModifications = 0;
	requiredModifications |= CheckLocalPart::getRequiredModifications()
			| CheckUnautenticatedSenderRcpt::getRequiredModifications()
			| DnsBlCheck::getRequiredModifications()
			| PostmasterAddressCheck::getRequiredModifications();

	desc.xxfi_flags = requiredModifications;
	if (supportedProcesses & MilterHandler::CONNECT) {
		desc.xxfi_connect = &Milter::connect;
	}
	if (supportedProcesses & MilterHandler::HELO) {
		desc.xxfi_helo = &Milter::helo;
	}
	if (supportedProcesses & MilterHandler::FROM) {
		desc.xxfi_envfrom = &Milter::envfrom;
	}
	if (supportedProcesses & MilterHandler::RCPT) {
		desc.xxfi_envrcpt = &Milter::envrcpt;
	}
	if (supportedProcesses & MilterHandler::HEADER) {
		desc.xxfi_header = &Milter::header;
	}
	if (supportedProcesses & MilterHandler::EOH) {
		desc.xxfi_eoh = &Milter::eoh;
	}
	if (supportedProcesses & MilterHandler::BODY) {
		throw std::runtime_error("BODY not yet implemented!");
	}
	if (supportedProcesses & MilterHandler::EOM) {
		desc.xxfi_eom = &Milter::eom;
	}
	if (supportedProcesses & MilterHandler::ABORT) {
		desc.xxfi_abort = &Milter::abort;
	}
	if (supportedProcesses & MilterHandler::CLOSE) {
		desc.xxfi_close = &Milter::close;
	}
}

void Milter::addMilterHandlerToOperation(
		const MilterHandler::Operations operation, const short priority,
		std::shared_ptr<MilterHandler>& milterHandler) {
	MilterOperationsMap::iterator operationIt = milterHandlers.find(operation);
	if (operationIt == milterHandlers.end()) {
		operationIt = milterHandlers.emplace(operation,
				MilterHandlersPriorityMap()).first;
	}
	MilterHandlersPriorityMap::iterator prioIt = operationIt->second.find(
			priority);
	if (prioIt == operationIt->second.end()) {
		prioIt =
				operationIt->second.emplace(priority, MilterHandlersList()).first;
	}
	prioIt->second.push_back(milterHandler);
	LOG_DEBUG() << "Added " << typeid(*milterHandler).name()
			<< " to the milter for operation " << operation << " with priority "
			<< priority;
}

inline sfsistat Milter::processCommand(SMFICTX* ctx,
		MilterHandler::OperationsType operation,
		std::function<MilterHandler::MilterResult(MilterHandler&)> func) {
	return processCommand(ctx, SMFIS_CONTINUE, operation, func);
}

sfsistat Milter::processCommand(SMFICTX* ctx, sfsistat defaultStatus,
		MilterHandler::OperationsType operation,
		std::function<MilterHandler::MilterResult(MilterHandler&)> func) {
	MilterHandler::MilterResult ret = 0;
	MilterOperationsMap::iterator operations = milterHandlers.find(operation);
	if (operations == milterHandlers.end()) {
		LOG_DEBUG() << "Nobody registered to handle operation " << operation;
		return defaultStatus;
	}

	milterContext = ctx;
	SetPointerToNullOnDesctruct<SMFICTX> _dummy1(milterContext);
	for (MilterHandlersPriorityMap::iterator lists = operations->second.begin();
			lists != operations->second.end(); ++lists) {
		for (MilterHandlersList::iterator milterHandler = lists->second.begin();
				milterHandler != lists->second.end(); ++milterHandler) {
			LOG_DEBUG() << "Ask " << typeid(**milterHandler).name()
					<< " to handle operation " << operation;
			ret = func(**milterHandler);
			if (ret->status
					!= MilterHandler::MILTER_OPERATION_NO_RESULT->status) {
				LOG_DEBUG() << "Operation " << operation << " handled by "
						<< typeid(**milterHandler).name() << " with result "
						<< ret->status << "(" << ret->rcode << " " << ret->xcode
						<< " " << ret->msg << ")";
				std::shared_ptr<char> rcode;
				std::shared_ptr<char> xcode;
				std::shared_ptr<char> msg;
				char* rcodePtr = NULL;
				char* xcodePtr = NULL;
				char* msgPtr = NULL;
				if (!ret->rcode.empty()) {
					rcode = copyStringToArray(ret->rcode);
					rcodePtr = rcode.get();
				}
				if (!ret->xcode.empty()) {
					xcode = copyStringToArray(ret->xcode);
					xcodePtr = xcode.get();
				}
				if (!ret->msg.empty()) {
					msg = copyStringToArray(ret->msg);
					msgPtr = msg.get();
				}
				if (rcodePtr != NULL || xcodePtr != NULL || msgPtr != NULL) {
					LOG_DEBUG() << "Set reply. rcode=" << rcodePtr
							<< " ; xcode=" << xcodePtr << " ; msg=" << msgPtr;
					smfi_setreply(ctx, rcodePtr, xcodePtr, msgPtr);
				}
				return ret->status;
			}
		}
	}

	LOG_DEBUG() << "Nobody handled operation " << operation;

	return defaultStatus;
}

sfsistat Milter::connect(SMFICTX* ctx, char* hostname,
_SOCK_ADDR* hostaddr) {
	LOG_DEBUG() << "New connect from " << hostname;
	Milter* milter = (Milter*) smfi_getpriv(ctx);
	if (milter == NULL) {
		milter = new Milter();
		smfi_setpriv(ctx, milter);

	}
	milter->remoteHostname.assign(hostname);
	memcpy(&(milter->remoteAddress), hostaddr, sizeof(milter->remoteAddress));
	return milter->processCommand(ctx, MilterHandler::CONNECT,
			&MilterHandler::connect);
}

sfsistat Milter::helo(SMFICTX* ctx, char* heloName) {
	std::string heloNameStr(heloName);
	return getMilterFromContext(ctx).processCommand(ctx, MilterHandler::HELO,
			[heloNameStr](MilterHandler& mh) {return mh.helo(heloNameStr);});
}

sfsistat Milter::envfrom(SMFICTX* ctx, char** argv) {
	Milter& milter = getMilterFromContext(ctx);
	milter.mailFrom.assign(argv[0]);
	if (milter.mailFrom.front() == '<' && milter.mailFrom.back() == '>') {
		milter.mailFrom = milter.mailFrom.substr(1,
				milter.mailFrom.length() - 2);
	}
	return milter.processCommand(ctx, MilterHandler::FROM,
			&MilterHandler::envfrom);
}

sfsistat Milter::envrcpt(SMFICTX* ctx, char** argv) {
	std::string rcpt(argv[0]);
	if (rcpt.front() == '<' && rcpt.back() == '>') {
		rcpt = rcpt.substr(1, rcpt.length() - 2);
	}
	Milter& milter = getMilterFromContext(ctx);
	milter.mailRecipients.insert(rcpt);
	return milter.processCommand(ctx, MilterHandler::RCPT,
			[rcpt](MilterHandler& mh) {return mh.envrcpt(rcpt);});
}

sfsistat Milter::data(SMFICTX* ctx) {
	Milter& milter = getMilterFromContext(ctx);
	return getMilterFromContext(ctx).processCommand(ctx, MilterHandler::DATA,
			&MilterHandler::data);
}

sfsistat Milter::header(SMFICTX* ctx, char* headerf, char* headerv) {
	std::string headerField(headerf);
	std::string headerValue(headerv);
	return getMilterFromContext(ctx).processCommand(ctx, MilterHandler::HEADER,
			[headerField,headerValue](MilterHandler& mh) {return mh.header(headerField,headerValue);});
}

sfsistat Milter::eoh(SMFICTX* ctx) {
	return getMilterFromContext(ctx).processCommand(ctx, MilterHandler::EOH,
			&MilterHandler::eoh);
}

sfsistat Milter::eom(SMFICTX* ctx) {
	return getMilterFromContext(ctx).processCommand(ctx, MilterHandler::EOM,
			&MilterHandler::eom);
}

sfsistat Milter::abort(SMFICTX* ctx) {
	return getMilterFromContext(ctx).processCommand(ctx, MilterHandler::ABORT,
			&MilterHandler::abort);
}

sfsistat Milter::close(SMFICTX* ctx) {
	if (smfi_getpriv(ctx) == NULL) {
		return SMFIS_CONTINUE;
	}
	std::unique_ptr<Milter> milter((Milter*) smfi_getpriv(ctx));
	LOG_DEBUG() << "Close for connection from "
			<< milter->getMailFrom().c_str();
	smfi_setpriv(ctx, NULL);
	return milter->processCommand(ctx, MilterHandler::CLOSE,
			&MilterHandler::close);
}

sfsistat Milter::negotiate(SMFICTX *ctx, unsigned long f0, unsigned long f1,
		unsigned long f2, unsigned long f3, unsigned long *pf0,
		unsigned long *pf1, unsigned long *pf2, unsigned long *pf3) {
	return SMFIS_CONTINUE;
}
