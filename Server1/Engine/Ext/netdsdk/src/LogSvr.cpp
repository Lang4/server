#include "LogSvr.h"
#include "Debug.h"
#include "Thread.h"
#include "Guard_T.h"
#include "GlobalMacros.h"

#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdarg.h>
#include <errno.h>

LogSvr::LogSvr ()
{
    log_buff_      = new char[MAX_LOGSTR_LEN+1];
    log_file_name_ = new char[MAX_LOGFILE_NAME_LEN+1];
    log_dir_       = new char[MAX_LOGFILE_NAME_LEN+1];
    log_time_      = new char[LOG_TIME_STR_LEN+1];
    log_handle_    = NDK_INVALID_HANDLE;
    for (int i = 0; i < LOG_ITEMS; ++i)
    {
	log_items_[i] = new char[16];
	memset (log_items_[i], 0, 16);
    }
    /// 
    strcpy (log_items_[LOG_TRACE], "LOG_TRACE");
    strcpy (log_items_[LOG_DEBUG], "LOG_DEBUG");
    strcpy (log_items_[LOG_RINFO], "LOG_RINFO");
    strcpy (log_items_[LOG_ERROR], "LOG_ERROR"); 

    memset (log_buff_, 0, MAX_LOGSTR_LEN);
    log_buff_[MAX_LOGSTR_LEN] = '\0';

    memset (log_file_name_, 0, MAX_LOGFILE_NAME_LEN);
    log_file_name_[MAX_LOGFILE_NAME_LEN] = '\0';

    memset (log_dir_, 0, MAX_LOGFILE_NAME_LEN);
    log_dir_[MAX_LOGFILE_NAME_LEN] = '\0';

    memset (log_time_, 0, LOG_TIME_STR_LEN);
    log_time_[LOG_TIME_STR_LEN] = '\0';
    
    /// get tomorrow 00:00:00 
    current_dtime_.update ();
    current_dtime_.mday (current_dtime_.mday () + 1);
    current_dtime_.hour (0);
    current_dtime_.min (0);
    current_dtime_.sec (0);
    day_start_time_ = current_dtime_.time ();
}
LogSvr::~LogSvr ()
{
    if (this->log_buff_)
	delete []this->log_buff_;
    if (this->log_file_name_)
	delete []this->log_file_name_;
    if (this->log_dir_)
	delete []this->log_dir_;
    if (this->log_items_)
    {
	for (int i = 0; i < LOG_ITEMS; ++i)
	    delete []log_items_[i];
    }
}
int LogSvr::open (const char *log_dir)
{
    Guard_T<MUTEX> guard (this->log_mutex_);
    if (mkdir (log_dir, 0755) == -1)
    {
	if (errno != EEXIST)
	{
	    return -1;
	}
	errno = 0;
    }
    strcpy (this->log_dir_, log_dir); 
    memset (this->log_file_name_, 0, MAX_LOGFILE_NAME_LEN);
    char date_str[32] = {0};
    this->current_dtime_.update ();
    this->current_dtime_.date_to_str(date_str, sizeof (date_str));
    snprintf (this->log_file_name_, MAX_LOGFILE_NAME_LEN, "%s/%s.log", 
	    this->log_dir_, date_str);
    this->log_handle_ = ::open (this->log_file_name_, 
	    O_CREAT | O_RDWR | O_APPEND, 0644);
    if (this->log_handle_ == NDK_INVALID_HANDLE) // create file failed
    {
	return -1;
    }
    return 0;
}
void LogSvr::put (size_t logtype, const char *format, ...) 
{
    if (this->log_handle_ == NDK_INVALID_HANDLE)
	return ;
    Guard_T<MUTEX> guard (this->log_mutex_);
    va_list argptr;
    int len = 0;
    va_start (argptr, format);
    memset (this->log_buff_, 0, MAX_LOGSTR_LEN);

    /// avoid more than LOG_ITEMS , if logtype < 0 , logtype % LOG_ITEMS == 0
    logtype %= LOG_ITEMS;        
    
    this->current_time_.gettimeofday ();
    this->current_dtime_.update (this->current_time_.sec ());
    this->current_dtime_.time_to_str(this->log_time_, LOG_TIME_STR_LEN);
    this->current_time_.sec (0);
    int ret = snprintf (this->log_buff_, MAX_LOGSTR_LEN, "[%s.%lu][%lu]<%s>: ", 
	    this->log_time_, 
	    this->current_time_.msec (),
	    Thread::self (),
	    this->log_items_[logtype]);
    len += ret;
    ret = vsnprintf (this->log_buff_ + len, MAX_LOGSTR_LEN - len, format, argptr);
    if (ret < 0) return ;
    /**
     * check overflow or not
     * Note : snprintf and vnprintf return value is the number of characters 
     * (not including the trailing ’\0’) which would have been  written  to  
     * the  final  string  if enough space had been available
     */
    if (ret >= (MAX_LOGSTR_LEN - len))  
	len += MAX_LOGSTR_LEN - len - 1;  
	// vsnprintf return the length of <argptr> actual
    else
	len += ret;
					 // so 
    this->check_log_time ();
    if (::write (this->log_handle_, this->log_buff_, len) <= 0)
    {
	this->log_handle_ = NDK_INVALID_HANDLE;
    }
    return ;
}

void LogSvr::check_log_time () 
{
    if (difftime (this->current_dtime_.time (), this->day_start_time_) >= 0)   // next day
    {
	memset (this->log_file_name_, 0, MAX_LOGFILE_NAME_LEN);
	char date_str[32] = {0};
	this->current_dtime_.date_to_str(date_str, sizeof (date_str));
	snprintf (this->log_file_name_, MAX_LOGFILE_NAME_LEN, "%s/%s.log", 
		this->log_dir_, date_str);
	int handle = ::open (this->log_file_name_, 
		O_CREAT | O_RDWR | O_APPEND, 0644);
	if (handle != NDK_INVALID_HANDLE) // create file failed
	{
	    ::close (this->log_handle_);  // close yestoday log
	    this->log_handle_ = handle; // 
	}
	this->current_dtime_.mday (this->current_dtime_.mday () + 1);
	this->current_dtime_.hour (0);
	this->current_dtime_.min (0);
	this->current_dtime_.sec (0);
	this->day_start_time_ = current_dtime_.time ();
    }
    return ;
}

