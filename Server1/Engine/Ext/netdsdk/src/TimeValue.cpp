#include "TimeValue.h"

const TimeValue TimeValue::zero;

void TimeValue::normalize ()
{
    if (this->tv_.tv_usec >= 1000000)
    {
	do
	{
	    ++this->tv_.tv_sec;
	    this->tv_.tv_usec -= 1000000;
	}while (this->tv_.tv_usec >= 1000000);
    }else if (this->tv_.tv_usec <= -1000000)
    {
	do
	{
	    --this->tv_.tv_sec;
	    this->tv_.tv_usec += 1000000;
	}while (this->tv_.tv_usec <= -1000000);
    }

    if (this->tv_.tv_sec >= 1 && this->tv_.tv_usec < 0)
    {
	--this->tv_.tv_sec;
	this->tv_.tv_usec += 1000000;
    }
}

