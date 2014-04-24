#include "LTime.h"
#ifdef WIN32
#   include <windows.h>
#else
#   include <sys/time.h>
#endif

#ifdef WIN32
int gettimeofday(struct timeval *tp, void *tzp)
{
	time_t clock;
	struct tm tm;
	SYSTEMTIME wtm;

	GetLocalTime(&wtm);
	tm.tm_year     = wtm.wYear - 1900;
	tm.tm_mon     = wtm.wMonth - 1;
	tm.tm_mday     = wtm.wDay;
	tm.tm_hour     = wtm.wHour;
	tm.tm_min     = wtm.wMinute;
	tm.tm_sec     = wtm.wSecond;
	tm. tm_isdst    = -1;
	clock = mktime(&tm);
	tp->tv_sec = clock;
	tp->tv_usec = wtm.wMilliseconds * 1000;

	return (0);
}
#endif
/**
 * \brief 得到系统时区设置字符串
 *
 * \param s 时区将放入此字符串中
 * \return 返回参数s
 */
std::string & LRTime::getLocalTZ(std::string & s)
{
  long tz;

  std::ostringstream so;
  tzset();
  tz = _timezone/3600;
  //so << _tzname[0];
  so << tz;
  s= so.str();
  return s;
}
