#pragma once

#include <log4cpp/Category.hh>
#include <typeinfo>

#define LOG_LOGGERVAR() \
	static log4cpp::Category& logger;

#define LOG_LOGGERINIT(clazz,name) \
	log4cpp::Category& clazz::logger = log4cpp::Category::getInstance( name );

#define LOG_EXCEPT(exceptionToLog) \
	"(" << typeid(exceptionToLog).name() << ") " << exceptionToLog.what()

#define LOG_EXCEPT_PTR(exceptionToLog) \
	"(" << typeid(*exceptionToLog).name() << ") " << exceptionToLog->what()

#define LOG_EMERG() \
   if (logger.isEmergEnabled()) logger.emergStream()

/*#define LOG_FATAL() \
   if (logger.isFatalEnabled()) logger.fatalStream() << typeid(*this) << ": "

 #define LOG_ALERT() \
   if (logger.isAlertEnabled()) logger.alertStream() << typeid(*this) << ": "

 #define LOG_CRIT() \
   if (logger.isCritEnabled()) logger.critStream() << typeid(*this) << ": "*/

#define LOG_ERROR() \
   if (logger.isErrorEnabled()) logger.errorStream()

#define LOG_WARN() \
   if (logger.isWarnEnabled()) logger.warnStream()

/*#define LOG_NOTICE() \
   if (logger.isNoticeEnabled()) logger.noticeStream() << typeid(*this) << ": "*/

#define LOG_INFO() \
   if (logger.isInfoEnabled()) logger.infoStream()

#define LOG_DEBUG() \
   if (logger.isDebugEnabled()) logger.debugStream()
