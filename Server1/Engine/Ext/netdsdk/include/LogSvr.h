//========================================================================
/**
 * Author   : cuisw <shaovie@gmail.com>
 * Date     : 2007-12-18 20:28
 */
//========================================================================

#ifndef _LOGSVR_H_
#define _LOGSVR_H_
#include "Pre.h"

#include "DateTime.h"
#include "TimeValue.h"
#include "Singleton_T.h"
#include "ThreadMutex.h"

#define LOG_TIME_STR_LEN        32
#define MAX_LOGSTR_LEN          511
#define MAX_LOGFILE_NAME_LEN    31

enum {
    LOG_TRACE = 0,
    LOG_DEBUG,
    LOG_RINFO,
    LOG_ERROR,
    LOG_ITEMS
};

/**
 * @class LogSvr
 *
 * @brief Spawn (1) thread to write log , use thread avoid to block write
 */
class LogSvr
{
    friend class Singleton_T<LogSvr, ThreadMutex>;
public:
    /// create log file and dir
    int open (const char *log_dir);

    /// logtype : LOG_TRACE LOG_DEBUG LOG_RINFO LOG_ERROR
    void put (size_t log_type, const char *format, ...);
protected:
    LogSvr ();
    ~LogSvr ();

    /// check logtime for one day one log-file, 
    void check_log_time ();
private:
    int              log_handle_;
    char*	     log_buff_;
    char*            log_dir_;
    char*	     log_file_name_;
    char*            log_time_;
    char*	     log_items_[LOG_ITEMS];
    time_t           day_start_time_;
    DateTime	     current_dtime_;
    TimeValue	     current_time_;
    typedef ThreadMutex MUTEX;
    MUTEX      log_mutex_;
};

#include "Post.h"
#endif

