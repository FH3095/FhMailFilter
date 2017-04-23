#pragma once

#include <libmilter/mfapi.h>
#include "../Util.h"
#include "MilterHandler.h"
#include <cstdint>
#include <unordered_map>
#include <map>
#include <list>
#include <string>
#include <functional>
#include <memory>

class Milter {
private:
	LOG_LOGGERVAR();
	static const std::string REQUESTED_MACROS;
	typedef std::list<std::shared_ptr<MilterHandler>> MilterHandlersList;
	typedef std::map<short, MilterHandlersList> MilterHandlersPriorityMap;
	typedef std::unordered_map<MilterHandler::OperationsType,
	MilterHandlersPriorityMap> MilterOperationsMap;
	template <typename PtrType>
	class SetPointerToNullOnDesctruct {
	private:
		PtrType*& ptr;
	public:
		SetPointerToNullOnDesctruct(PtrType*& ptr) : ptr(ptr) {
		}
		~SetPointerToNullOnDesctruct() {
			ptr=NULL;
		}
	};
	MilterOperationsMap milterHandlers;
	_SOCK_ADDR remoteAddress;
	std::string remoteHostname;
	std::string mailFrom;
	std::set<std::string> mailRecipients;
	SMFICTX* milterContext;

	void addMilterHandlerToOperation(const MilterHandler::Operations operation,const short priority,std::shared_ptr<MilterHandler>& milterHandler);
	static inline Milter& getMilterFromContext(SMFICTX* ctx);
	inline sfsistat processCommand(SMFICTX* ctx,MilterHandler::OperationsType operation,std::function<MilterHandler::MilterResult(MilterHandler&)> func);
	sfsistat processCommand(SMFICTX* ctx,sfsistat defaultStatus,MilterHandler::OperationsType operation,std::function<MilterHandler::MilterResult(MilterHandler&)> func);
public:
	static const short PRIO_FIRST_READ;
	static const short PRIO_FIRST_MODIFY;
	static const short PRIO_MIDDLE;
	static const short PRIO_LAST_MODIFY;
	static const short PRIO_LAST_READ;

	Milter();
	virtual ~Milter();

	static void fillMilterStruct(smfiDesc& desc);

	static sfsistat connect(SMFICTX* ctx, char* hostname, _SOCK_ADDR* hostaddr);
	static sfsistat helo(SMFICTX* ctx, char* heloHost);
	static sfsistat envfrom(SMFICTX* ctx, char** argv);
	static sfsistat envrcpt(SMFICTX* ctx, char** argv);
	static sfsistat data(SMFICTX* ctx);
	static sfsistat header(SMFICTX* ctx, char* headerf, char* headerv);
	static sfsistat eoh(SMFICTX* ctx);
	static sfsistat eom(SMFICTX* ctx);
	static sfsistat abort(SMFICTX* ctx);
	static sfsistat close(SMFICTX* ctx);
	static sfsistat negotiate(SMFICTX *ctx,
			unsigned long f0, unsigned long f1,
			unsigned long f2, unsigned long f3,
			unsigned long* pf0, unsigned long* pf1,
			unsigned long* pf2, unsigned long* pf3);

	inline const _SOCK_ADDR* getRemoteAddress() {
		return &remoteAddress;
	}

	inline const std::string& getRemoteHostname() {
		return remoteHostname;
	}

	inline const std::string& getMailFrom() {
		return mailFrom;
	}

	inline const std::set<std::string> getMailRecipients() {
		return mailRecipients;
	}

	inline SMFICTX* getMilterContext() {
		return milterContext;
	}
};
