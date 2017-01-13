/*************************************************************************
    > File Name: log.cpp
    > Author: sudoku.huang
    > Mail: sudoku.huang@gmail.com 
    > Created Time: Thursday, December 22, 2016 AM08:58:20 CST
 ************************************************************************/

#include "log.h"
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/time.h>

#include <utools/ufs.h>
#include <ulog/ulog.h>

#define U_SECONDS_PER_MIN   60
#define U_MINUTES_PER_HOUR  60
#define U_HOURS_PER_DAY     24

#define U_SECONDS_PER_HOUR  (U_SECONDS_PER_MIN * U_MINUTES_PER_HOUR)
#define U_SECONDS_PER_DAY   (U_HOURS_PER_DAY * U_SECONDS_PER_HOUR)

CLbsLog * CLbsLog::sinstance = NULL;

void CLbsLog::log_hook(int level, const char * fmt, va_list valist)
{
    sinstance->log(level, fmt, valist);
}

CLbsLog::CLbsLog(const std::string & path, const std::string & name, int level)
    : path_(path)
    , name_(name)
    , level_(level)
    , log_fd_(-1)
    , log_buf_(NULL)
    , log_buf_size_(2048)
    , last_logfile_t_(0)
    , off_(8 * U_SECONDS_PER_HOUR + 2 * U_SECONDS_PER_MIN)
{
    log_buf_    = (char *)malloc(log_buf_size_);
    pthread_mutex_init(&log_mutex_, NULL);

    sinstance = this;
}

CLbsLog::~CLbsLog()
{
    sinstance = NULL;
    free(log_buf_);
    log_buf_ = NULL;

    if (log_fd_ > 0) {
        close(log_fd_);
        log_fd_ = -1;
    }
    
    pthread_mutex_destroy(&log_mutex_);

}

bool CLbsLog::init()
{
    if (makedir(path_.c_str())) {
        return false;
    }

    setulog(CLbsLog::log_hook);

    return true;
}

void CLbsLog::setlevel(int level)
{
    if (getstringbylevel(level) == NULL) {
        return;
    }

    level_ = level;
}

bool CLbsLog::reopen()
{
    if (log_fd_ > 0) {
        close(log_fd_);
        log_fd_ = -1;
    }

    std::string logfile = path_;
    logfile.append("/").append(name_);

    int openflag = O_CREAT;
    
    if (access(logfile.c_str(), F_OK) == 0) {
        
        time_t now  = time(NULL) + off_;
        time_t last = 0;
        if (last_logfile_t_ > 0) {
            last    = last_logfile_t_ + off_;
        }

        
        struct stat sb;
        int ret = stat(logfile.c_str(), &sb);
        
        //printf("off:%d\n", off_);
        //printf("last:%lu, now:%lu\n", last, now);
        //printf("stat: mtime:%lu\n", sb.st_mtime + off_);
        //printf("calc:%d, %d, %d\n", last / U_SECONDS_PER_DAY, 
        //    now / U_SECONDS_PER_DAY, 
        //    (sb.st_mtime + off_) / U_SECONDS_PER_DAY);
        
        if ((last != 0 && last / U_SECONDS_PER_DAY != now / U_SECONDS_PER_DAY) ||
            (ret == 0 && now / U_SECONDS_PER_DAY != (sb.st_mtime + off_) / U_SECONDS_PER_DAY && 
                now >= (sb.st_mtime + off_))) {

            struct tm tm_time;
            localtime_r(&sb.st_mtime, &tm_time);
        
            char buf[20];
            strftime(buf, sizeof(buf), "%Y%m%d", &tm_time); 

            std::string renamefile = logfile;
            renamefile.append(".").append(buf);

            int trytimes = 3;
                
            //printf("rename log  %s-%s\n", logfile.c_str(), renamefile.c_str());
            do {
                if (rename(logfile.c_str(), renamefile.c_str()) == 0) {
                    break;
                }
            } while (trytimes --);
        } else {
            openflag = 0;
        }
    }
    
    log_fd_         = open(logfile.c_str(), openflag | O_WRONLY, 0666);
    last_logfile_t_ = time(NULL);
    
    if (log_fd_ >= 0) {
        lseek(log_fd_, 0, SEEK_END);
    }
    
    return log_fd_ >= 0;
}

void CLbsLog::log(int level, const char * fmt, va_list valist)
{
    if (level_ < level) {
        return;
    }

    pthread_mutex_lock(&log_mutex_);
    size_t off = 0;
    
    time_t t_time = time(NULL);

    //printf("last:%lu, off_:%d, now:%lu\n", last_logfile_t_, off_, t_time);
    //printf("adjust. last:%lu, now:%lu\n", last_logfile_t_ + off_, t_time + off_);
    //printf("day, last:%lu, now:%lu\n", 
    //    (last_logfile_t_ + off_) / U_SECONDS_PER_DAY,
    //    (t_time + off_) / U_SECONDS_PER_DAY);
    if (log_fd_ < 0 || 
        (last_logfile_t_ + off_) / U_SECONDS_PER_DAY 
            != (t_time + off_) / U_SECONDS_PER_DAY) {
        if (!reopen()) {
            return;
        }
    }
    
    struct timeval tv;
    gettimeofday(&tv , NULL);
    
    struct tm tm_time;
    localtime_r(&t_time, &tm_time);
    //gmtime_r(&t_time, &tm_time);
    //tm_time.tm_hour += 8; // GMT -> CCT
    
    off = strftime(log_buf_, log_buf_size_, "%Y-%m-%d %H:%M:%S", 
        &tm_time); 
    
    off += snprintf(log_buf_ + off, log_buf_size_ - off, ":%.6d [P:%ld][%s] ", 
        (int)tv.tv_usec, pthread_self(), getstringbylevel(level));
        
    off += vsnprintf(log_buf_ + off, log_buf_size_ - off, fmt, valist);

    write(log_fd_, log_buf_, off);
    pthread_mutex_unlock(&log_mutex_);
}

