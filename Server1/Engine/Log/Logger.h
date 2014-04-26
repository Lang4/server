#pragma once
class LoggerInterface{
public:
	virtual void debug(const char * pattern, ...) = 0;
	virtual void info(const char * pattern, ...) = 0;
	virtual void error(const char * pattern, ...) = 0;
	virtual void warn(const char * pattern, ...) = 0;
};

extern LoggerInterface * GetLogger();