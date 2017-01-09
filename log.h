/*************************************************************************
    > File Name: log.h
    > Author: sudoku.huang
    > Mail: sudoku.huang@gmail.com 
    > Created Time: Thursday, December 22, 2016 AM08:58:26 CST
 ************************************************************************/

#ifndef __LBS_LOG_H__
#define __LBS_LOG_H__

#include <string>
#include <stdarg.h>

class CLbsLog
{
public:
    CLbsLog(const std::string & path, const std::string & name, int level);
    
    ~CLbsLog();
    
public:
    static void log_hook(int level, const char * fmt, va_list valist); 

    void log(int level, const char * fmt, va_list valist);

    bool init();

    void setlevel(int level);
    
private:
    bool reopen();

public:
    static CLbsLog * sinstance;
    
private:
    std::string path_;
    std::string name_;

    int     level_;
    int     log_fd_;
    char *  log_buf_;
    int     log_buf_size_;
    time_t  last_logfile_t_;
    pthread_mutex_t log_mutex_;
    int     off_;       //GMT Offset.
};

#endif