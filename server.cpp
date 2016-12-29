/*************************************************************************
    > File Name: server.cpp
    > Author: sudoku.huang
    > Mail: sudoku.huang@gmail.com 
    > Created Time: Thursday, December 22, 2016 AM08:58:15 CST
 ************************************************************************/

#include "server.h"
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <arpa/inet.h>
#include <strings.h>
#include <sys/time.h>
#include <stdio.h>

#include <ulog/ulog.h>
#include <utools/ufs.h>
#include "serverunit.h"

CLbsServer * CLbsServer::sinstance = NULL;

void CLbsServer::log_hook(int level, const char * fmt, va_list valist)
{
    sinstance->log(level, fmt, valist);
}

CLbsServer::CLbsServer(LbsConf_t & config)
    : config_(config)
    , listen_fd_(-1)
    , log_fd_(-1)
    , log_buf_(NULL)
    , log_buf_size_(2048)
    , last_logfile_t_(0)
    , server_units_()
    , area_db_()
    , ip_db_()
    , lbs_db_(area_db_)
{
    sinstance   = this;
    log_buf_    = (char *)malloc(log_buf_size_);
    pthread_mutex_init(&log_mutex_, NULL);
}

CLbsServer::~CLbsServer()
{
    sinstance = NULL;
    free(log_buf_);

    if (log_fd_ > 0) {
        close(log_fd_);
    }

    if (listen_fd_ > 0) {
        close(listen_fd_);
    }
    pthread_mutex_destroy(&log_mutex_);
}

CLbsDB & CLbsServer::lbsdb()
{
    return lbs_db_;
}

const CArea & CLbsServer::areadb() const
{
    return area_db_;
}

const CIPDB_CZ & CLbsServer::ipdb() const
{
    return ip_db_;
}

const LbsConf_t & CLbsServer::config() const
{
    return config_;
}

bool CLbsServer::run()
{
    if (!area_db_.load(config_.areadb.c_str())) {
        fprintf(stderr, "load area db:%s failed!\n", config_.areadb.c_str());
        return false;
    }

    if (!ip_db_.init(config_.ipdb.c_str())) {
        fprintf(stderr, "load ip db:%s failed!\n", config_.areadb.c_str());
        return false;
    }
    
    //init log.
    std::string logfile = config_.log_path; 
    if (makedir(logfile.c_str())) {
        fprintf(stderr, "create log director:%s failed!\n", logfile.c_str());
        return false;
    }
    
    setulog(CLbsServer::log_hook);
    ulog(ulog_error, "\n\nprogram start here!!!\n");

    if (!listen()) {
        ulog(ulog_error, "listen %s:%d failed!\n", 
            config_.server_ip.c_str(), config_.server_port);
        return false;
    }

    server_units_.resize(config_.threads);
    for (int i = 0; i < config_.threads; i++) {
        server_units_[i] = new CLbsServerUnit(*this);
        server_units_[i]->start();
    }

    run_forever();

    for (int i = 0; i < config_.threads; i++) {
        server_units_[i]->stop();
        delete server_units_[i];
    }
    server_units_.clear();
    
    return true;
}

void CLbsServer::run_forever()
{
    struct sockaddr_in  addr;
    socklen_t           addr_len = sizeof(addr);
    int fd;
    int idx = 0;
    
    while (1) {
        fd = accept(listen_fd_, (struct sockaddr *)&addr, &addr_len);

        if (fd > 0) {
            ulog(ulog_debug, "accept client, fd:%d, info:[%s:%d]\n",
                fd, inet_ntoa(addr.sin_addr) , ntohs(addr.sin_port));

            int workerid = idx ++ % server_units_.size();
            server_units_[workerid]->accept(fd, inet_ntoa(addr.sin_addr));
        }
    }
}

bool CLbsServer::listen()
{
    struct sockaddr_in server_addr;
    bzero(&server_addr,sizeof(server_addr));
    server_addr.sin_family      = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(config_.server_ip.c_str());
    server_addr.sin_port        = htons(config_.server_port);
 
    listen_fd_= socket(PF_INET, SOCK_STREAM, 0);
    if (listen_fd_ < 0) {
        return false;
    }

    int opt = 1;
    setsockopt(listen_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

     
    if (bind(listen_fd_, (struct sockaddr*)&server_addr, sizeof(server_addr))) {
        return false;
    }
    
    if (::listen(listen_fd_, 100)) {
        return false;
    }

    return true;
}

bool CLbsServer::reopen_log()
{
    if (log_fd_ > 0) {
        close(log_fd_);
        log_fd_ = -1;
    }

    std::string logfile = config_.log_path;
    logfile.append("/").append(config_.log_filename);

    int openflag = O_CREAT;
    
    if (access(logfile.c_str(), F_OK) == 0) {
        
        time_t now = time(NULL);
        
        struct stat sb;
        if ((last_logfile_t_ != 0 && last_logfile_t_ / 86400 != now / 86400) ||
            (!stat(logfile.c_str(), &sb) && 
                now / 86400 != sb.st_mtime / 86400 && 
                now > sb.st_mtime)) {
            
            struct tm tm_time;
            gmtime_r(&sb.st_mtime, &tm_time);
        
            char buf[20];
            strftime(buf, sizeof(buf), "%Y%m%d", &tm_time); 

            std::string renamefile = logfile;
            renamefile.append(".").append(buf);

            int trytimes = 3;
            
            do {
                if (rename(logfile.c_str(), renamefile.c_str()) == 0) {
                    break;
                }
            } while (trytimes --);
            
            last_logfile_t_ = now;
        } else {
            openflag = 0;
        }
    }
    
    log_fd_ = open(logfile.c_str(), openflag | O_WRONLY, 0666);

    if (log_fd_ >= 0) {
        lseek(log_fd_, 0, SEEK_END);
    }
    
    return log_fd_ >= 0;
}

void CLbsServer::log(int level, const char * fmt, va_list valist)
{
    if (config_.log_level < level) {
        return;
    }

    pthread_mutex_lock(&log_mutex_);
    size_t off = 0;
    
    time_t t_time = time(NULL);

    if (log_fd_ < 0 || last_logfile_t_ / 86400 != t_time / 86400) {
        if (!reopen_log()) {
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

