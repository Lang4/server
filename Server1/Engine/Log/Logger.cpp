#include "Logger.h"
#include <stdarg.h>
#include <string>
#include <log4cxx/logger.h>
#include <log4cxx/propertyconfigurator.h>
#include <log4cxx/helpers/exception.h>
using namespace log4cxx;
using namespace log4cxx::helpers;

#define msgFormatA(buf,msg)\
do{\
va_list ap;\
va_start(ap, msg);\
vsprintf_s(buf,sizeof(buf),msg,ap);\
va_end(ap);\
}while(0)

class Log4xcc_Logger:public LoggerInterface{
public:
	LoggerPtr rootLogger;
	Log4xcc_Logger()
	{
		rootLogger = Logger::getRootLogger();
		log4cxx::PropertyConfigurator::configure("log4cxx.properties");
	}
	virtual void debug(const char * pattern, ...)
	{
		char buf[1024] = {0};     
		msgFormatA(buf,pattern);          
		LOG4CXX_DEBUG(rootLogger,buf); 
	}
	virtual void info(const char * pattern, ...)
	{
		char buf[1024] = {0};     
		msgFormatA(buf,pattern);          
		LOG4CXX_INFO(rootLogger,buf); 
	}
	virtual void error(const char * pattern, ...)
	{
		char buf[1024] = {0};     
		msgFormatA(buf, pattern);          
		LOG4CXX_ERROR(rootLogger,buf); 
	}
	virtual void warn(const char * pattern, ...)
	{
		char buf[1024] = {0};     
		msgFormatA(buf, pattern);          
		LOG4CXX_WARN(rootLogger,buf); 
	}
};

Log4xcc_Logger logger;
LoggerInterface * GetLogger()
{
	return &logger;
}