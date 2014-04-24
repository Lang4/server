#pragma once
#include "windows.h"
#include "LType.h"
#include "LCantCopy.h"
#include <string>
#include <list>
#include <map>
#include <vector>
#include <time.h>
#include <sstream>
#include <errno.h>

extern int gettimeofday(struct timeval *tp, void *tzp);
/**
* \brief ʱ�䶨��
*
* 
*/
/**
* \brief ��ʵʱ����,��timeval�ṹ�򵥷�װ,�ṩһЩ����ʱ�亯��
* ʱ�侫�Ⱦ�ȷ�����룬
* ����timeval��man gettimeofday
*/
class LRTime
{

private:

	/**
	* \brief ��ʵʱ�任��Ϊ����
	*
	*/
	QWORD _msecs;

	/**
	* \brief �õ���ǰ��ʵʱ��
	*
	* \return ��ʵʱ�䣬��λ����
	*/
	QWORD _now()
	{
		QWORD retval = 0LL;
		struct timeval tv;
		gettimeofday(&tv,NULL);
		retval = tv.tv_sec;
		retval *= 1000;
		retval += tv.tv_usec / 1000;
		return retval;
	}

	/**
	* \brief �õ���ǰ��ʵʱ���ӳٺ��ʱ��
	* \param delay �ӳ٣�����Ϊ��������λ����
	*/
	void nowByDelay(int delay)
	{
		_msecs = _now();
		addDelay(delay);
	}

public:

	/**
	* \brief ���캯��
	*
	* \param delay ���������ʱ�����ʱ����λ����
	*/
	LRTime(const int delay = 0)
	{
		nowByDelay(delay);
	}

	/**
	* \brief �������캯��
	*
	* \param rt ����������
	*/
	LRTime(const LRTime &rt)
	{
		_msecs = rt._msecs;
	}

	/**
	* \brief ��ȡ��ǰʱ��
	*
	*/
	void now()
	{
		_msecs = _now();
	}

	/**
	* \brief ��������
	*
	* \return ����
	*/
	DWORD sec() const
	{
		return _msecs / 1000;
	}

	/**
	* \brief ���غ�����
	*
	* \return ������
	*/
	DWORD msec() const
	{
		return _msecs % 1000;
	}

	/**
	* \brief �����ܹ��ĺ�����
	*
	* \return �ܹ��ĺ�����
	*/
	QWORD msecs() const
	{
		return _msecs;
	}

	/**
	* \brief �����ܹ��ĺ�����
	*
	* \return �ܹ��ĺ�����
	*/
	void setmsecs(QWORD data)
	{
		_msecs = data;
	}

	/**
	* \brief ���ӳ�ƫ����
	*
	* \param delay �ӳ٣�����Ϊ��������λ����
	*/
	void addDelay(int delay)
	{
		_msecs += delay;
	}

	/**
	* \brief ����=�������
	*
	* \param rt ����������
	* \return ��������
	*/
	LRTime & operator= (const LRTime &rt)
	{
		_msecs = rt._msecs;
		return *this;
	}

	/**
	* \brief �ع�+������
	*
	*/
	const LRTime & operator+ (const LRTime &rt)
	{
		_msecs += rt._msecs;
		return *this;
	}

	/**
	* \brief �ع�-������
	*
	*/
	const LRTime & operator- (const LRTime &rt)
	{
		_msecs -= rt._msecs;
		return *this;
	}

	/**
	* \brief �ع�>���������Ƚ�zRTime�ṹ��С
	*
	*/
	bool operator > (const LRTime &rt) const
	{
		return _msecs > rt._msecs;
	}

	/**
	* \brief �ع�>=���������Ƚ�zRTime�ṹ��С
	*
	*/
	bool operator >= (const LRTime &rt) const
	{
		return _msecs >= rt._msecs;
	}

	/**
	* \brief �ع�<���������Ƚ�zRTime�ṹ��С
	*
	*/
	bool operator < (const LRTime &rt) const
	{
		return _msecs < rt._msecs;
	}

	/**
	* \brief �ع�<=���������Ƚ�zRTime�ṹ��С
	*
	*/
	bool operator <= (const LRTime &rt) const
	{
		return _msecs <= rt._msecs;
	}

	/**
	* \brief �ع�==���������Ƚ�zRTime�ṹ�Ƿ����
	*
	*/
	bool operator == (const LRTime &rt) const
	{
		return _msecs == rt._msecs;
	}

	/**
	* \brief ��ʱ�����ŵ�ʱ�䣬��λ����
	* \param rt ��ǰʱ��
	* \return ��ʱ�����ŵ�ʱ�䣬��λ����
	*/
	QWORD elapse(const LRTime &rt) const
	{
		if (rt._msecs > _msecs)
			return (rt._msecs - _msecs);
		else
			return 0LL;
	}

	static std::string & getLocalTZ(std::string & s);
	static void getLocalTime(struct tm & tv1,time_t timValue)
	{
		timValue +=8*60*60;
		tv1 = *gmtime(&timValue);
	}

};

/**
* \brief ʱ����,��struct tm�ṹ�򵥷�װ
*/

class LTime
{

public:

	/**
	* \brief ���캯��
	*/
	LTime()
	{
		time(&secs);
		LRTime::getLocalTime(tv,secs);
	}

	/**
	* \brief �������캯��
	*/
	LTime(const LTime &ct)
	{
		secs = ct.secs;
		LRTime::getLocalTime(tv,secs);
	}

	/**
	* \brief ��ȡ��ǰʱ��
	*/
	void now()
	{
		time(&secs);
		LRTime::getLocalTime(tv,secs);
	}

	/**
	* \brief ���ش洢��ʱ��
	* \return ʱ�䣬��
	*/
	time_t sec() const
	{
		return secs;
	}

	/**
	* \brief ����=�������
	* \param rt ����������
	* \return ��������
	*/
	LTime & operator= (const LTime &rt)
	{
		secs = rt.secs;
		return *this;
	}

	/**
	* \brief �ع�+������
	*/
	const LTime & operator+ (const LTime &rt)
	{
		secs += rt.secs;
		return *this;
	}

	/**
	* \brief �ع�-������
	*/
	const LTime & operator- (const LTime &rt)
	{
		secs -= rt.secs;
		return *this;
	}

	/**
	* \brief �ع�-������
	*/
	const LTime & operator-= (const time_t s)
	{
		secs -= s;
		return *this;
	}

	/**
	* \brief �ع�>���������Ƚ�zTime�ṹ��С
	*/
	bool operator > (const LTime &rt) const
	{
		return secs > rt.secs;
	}

	/**
	* \brief �ع�>=���������Ƚ�zTime�ṹ��С
	*/
	bool operator >= (const LTime &rt) const
	{
		return secs >= rt.secs;
	}

	/**
	* \brief �ع�<���������Ƚ�zTime�ṹ��С
	*/
	bool operator < (const LTime &rt) const
	{
		return secs < rt.secs;
	}

	/**
	* \brief �ع�<=���������Ƚ�zTime�ṹ��С
	*/
	bool operator <= (const LTime &rt) const
	{
		return secs <= rt.secs;
	}

	/**
	* \brief �ع�==���������Ƚ�zTime�ṹ�Ƿ����
	*/
	bool operator == (const LTime &rt) const
	{
		return secs == rt.secs;
	}

	/**
	* \brief ��ʱ�����ŵ�ʱ�䣬��λ��
	* \param rt ��ǰʱ��
	* \return ��ʱ�����ŵ�ʱ�䣬��λ��
	*/
	time_t elapse(const LTime &rt) const
	{
		if (rt.secs > secs)
			return (rt.secs - secs);
		else
			return 0;
	}

	/**
	* \brief ��ʱ�����ŵ�ʱ�䣬��λ��
	* \return ��ʱ�����ŵ�ʱ�䣬��λ��
	*/
	time_t elapse() const
	{
		LTime rt;
		return (rt.secs - secs);
	}

	/**
	* \brief �õ���ǰ���ӣ���Χ0-59��
	*
	* \return 
	*/
	int getSec()
	{
		return tv.tm_sec;
	}

	/**
	* \brief �õ���ǰ���ӣ���Χ0-59��
	*
	* \return 
	*/
	int getMin()
	{
		return tv.tm_min;
	}

	/**
	* \brief �õ���ǰСʱ����Χ0-23��
	*
	* \return 
	*/
	int getHour()
	{
		return tv.tm_hour;
	}

	/**
	* \brief �õ���������Χ1-31
	*
	* \return 
	*/
	int getMDay()
	{
		return tv.tm_mday;
	}

	/**
	* \brief �õ���ǰ���ڼ�����Χ1-7
	*
	* \return 
	*/
	int getWDay()
	{
		return tv.tm_wday;
	}

	/**
	* \brief �õ���ǰ�·ݣ���Χ1-12
	*
	* \return 
	*/
	int getMonth()
	{
		return tv.tm_mon+1;
	}

	/**
	* \brief �õ���ǰ���
	*
	* \return 
	*/
	int getYear()
	{
		return tv.tm_year+1900;
	}  

private:

	/**
	* \brief �洢ʱ�䣬��λ��
	*/
	time_t secs;

	/**
	* \brief tm�ṹ���������
	*/
	struct tm tv;


};

class Timer
{
public:
	Timer(const float how_long,const int delay=0) : _long((int)(how_long*1000)),_timer(delay*1000)
	{

	}
	Timer(const float how_long,const LRTime cur) : _long((int)(how_long*1000)),_timer(cur)
	{
		_timer.addDelay(_long);
	}
	void next(const LRTime &cur)
	{
		_timer=cur;
		_timer.addDelay(_long);
	} 
	bool operator() (const LRTime& current)
	{
		if (_timer <= current) {
			_timer = current;
			_timer.addDelay(_long);
			return true;
		}

		return false;
	}
private:
	int _long;
	LRTime _timer;
};
