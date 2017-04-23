#pragma once

#include <libmilter/mfapi.h>
#include <string>
#include <memory>
#include <list>
#include <sstream>
#include "../Util.h"

class MilterHandler {
private:
	LOG_LOGGERVAR();
public:
	typedef unsigned long OperationsType;
	enum Operations {
		CONNECT = 0x1,
		HELO = 0x2,
		FROM = 0x4,
		RCPT = 0x8,
		DATA = 0x10,
		HEADER = 0x20,
		EOH = 0x40,
		BODY = 0x80,
		EOM = 0x100,
		ABORT = 0x200,
		CLOSE = 0x400,
	};
	struct Result {
		sfsistat status;
		std::string rcode;
		std::string xcode;
		std::string msg;
		Result(sfsistat status) {
			this->status = status;
		}
		Result(std::string rcode, std::string xcode, std::string msg) :
		Result(status) {
			if (rcode.empty()) {
				LOG_ERROR() << "rcode is empty. xcode=" << xcode << " ; msg="
				<< msg;
				throw IllegalArgumentError("rcode is empty!");
			}
			if (stringStartsWith(rcode, "4")) {
				status = SMFIS_TEMPFAIL;
			} else if (stringStartsWith(rcode, "5")) {
				status = SMFIS_REJECT;
			} else {
				LOG_ERROR() << "rcode doesnt start with 4 or 5. rcode=" << rcode << " ; xcode=" << xcode << " ; msg=" << msg;
				throw IllegalArgumentError(
						"rcode doesnt start with 4 or 5. rcode=" + rcode
						+ " ; xcode=" + xcode + " ; msg=" + msg);
			}
			this->rcode = rcode;
			this->xcode = xcode;
			this->msg = msg;
		}
	};
	typedef std::shared_ptr<Result> MilterResult;
protected:
	virtual MilterResult constructResult(sfsistat status) {
		MilterResult ret(new Result(status));
		return ret;
	}
	virtual MilterResult constructResult(
			const std::string& rcode, const std::string& xcode,
			const std::stringstream& msg) {
		MilterResult ret(new Result(rcode,xcode,msg.str()));
		return ret;
	}
public:
	static const MilterResult MILTER_OPERATION_NO_RESULT;

	MilterHandler() {
	}

	virtual ~MilterHandler() {
	}

	virtual MilterResult connect() {
		return MILTER_OPERATION_NO_RESULT;
	}

	virtual MilterResult helo(const std::string& heloName) {
		return MILTER_OPERATION_NO_RESULT;
	}

	virtual MilterResult envfrom() {
		return MILTER_OPERATION_NO_RESULT;
	}

	virtual MilterResult envrcpt(const std::string& rcpt) {
		return MILTER_OPERATION_NO_RESULT;
	}

	virtual MilterResult data() {
		return MILTER_OPERATION_NO_RESULT;
	}

	virtual MilterResult header(const std::string& headerField,
			const std::string& headerValue) {
		return MILTER_OPERATION_NO_RESULT;
	}

	virtual MilterResult eoh() {
		return MILTER_OPERATION_NO_RESULT;
	}

	virtual MilterResult eom() {
		return MILTER_OPERATION_NO_RESULT;
	}

	virtual MilterResult abort() {
		return MILTER_OPERATION_NO_RESULT;
	}

	virtual MilterResult close() {
		return MILTER_OPERATION_NO_RESULT;
	}

	static std::string getMailLocalPart(const std::string& mail);
	static std::string getMailDomainPart(const std::string& mail);
};
