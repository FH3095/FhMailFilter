#include <iostream>
using namespace std;

#include <log4cpp/PropertyConfigurator.hh>
#include <libmilter/mfapi.h>
#include <typeinfo>
#include <memory>
#include <sodium.h>
#include <resolv.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "log.h"
#include "config/Config.h"
#include "milter/Milter.h"

class Main {
private:
	LOG_LOGGERVAR();
	static std::unique_ptr<char[]> filterListenAddress;
	static char filterName[];
public:
	int main(int argc,char** argv)
	{
		log4cpp::PropertyConfigurator::configure("log4cpp.properties");

		Config::readConfig();
		Config& config=Config::getInstance();

		res_init();
		if (sodium_init() == -1)
		{
			throw new std::runtime_error("Cant initialize libsodium");
		}

		smfi_setdbg(config.getMilterLogLevel());
		smfi_settimeout(1800);
		smfi_setbacklog(10);
		filterListenAddress.reset(new char[config.getListenAddress().size()+1]);
		memcpy(filterListenAddress.get(),config.getListenAddress().c_str(),config.getListenAddress().size());
		filterListenAddress.get()[config.getListenAddress().size()] = '\0';
		smfi_setconn(filterListenAddress.get());

		smfiDesc desc;
		memset(&desc,0,sizeof(desc));
		desc.xxfi_name = filterName;
		desc.xxfi_version = SMFI_VERSION;
		Milter::fillMilterStruct(desc);
		smfi_register(desc);

		// Now open socket
		LOG_INFO() << "Opening milter socket on " << filterListenAddress.get();
		mode_t preSocketMask=umask(config.getSocketUMask());
		int ret=smfi_opensocket(true);
		umask(preSocketMask);
		if(MI_SUCCESS!=ret) {
			LOG_EMERG() << "opensocket failed with " << ret;
			throw std::runtime_error("Cant open socket");
		}

		// Drop privileges
		if (setgid(config.getGroupId()) != 0) {
			LOG_ERROR() << "setgid: Unable to drop group privileges: " << strerror(errno);
			exit(1);
		}
		if (setuid(config.getUserId()) != 0) {
			LOG_ERROR() << "setuid: Unable to drop user privileges: " << strerror(errno);
			exit(1);
		}
		if (setuid(0) != -1) {
			LOG_ERROR() << "Managed to regain root privileges";
			exit(1);
		}

		ret=smfi_main();
		LOG_INFO() << "main exited with " << ret;

		log4cpp::Category::shutdown();

		return ret;
	}
};

char Main::filterName[] = "FhMilter";
std::unique_ptr<char[]> Main::filterListenAddress;

LOG_LOGGERINIT(Main, "main");

int main(int argc, char* argv[]) {
	try {
		return Main().main(argc, argv);
	} catch (std::exception& e) {
		log4cpp::Category::getRoot().emergStream() << "Error while running: "
				<< LOG_EXCEPT(e);
		throw e;
	}
}
