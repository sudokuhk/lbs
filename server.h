/*************************************************************************
    > File Name: server.h
    > Author: sudoku.huang
    > Mail: sudoku.huang@gmail.com 
    > Created Time: Thursday, December 22, 2016 AM08:58:32 CST
 ************************************************************************/

#ifndef __LBS_SERVER_H__
#define __LBS_SERVER_H__

#include <stdarg.h>
#include <string>
#include <time.h>
#include <vector>
#include <map>
#include <set>

typedef struct SLbsConf
{
    //server
    std::string     server_ip;
    int             server_port;
    int             threads;
    int             deamon;
    std::string     areadb;
    std::string     ipdb;

    //log
    int             log_level;
    std::string     log_path;
    std::string     log_filename;

} LbsConf_t;

class CLbsServerUnit;

class CLbsServer
{   
public:
    CLbsServer(LbsConf_t & config);

    ~CLbsServer();
    
    bool run();

    static void log_hook(int level, const char * fmt, va_list valist); 

private:
    void run_forever();

    void log(int level, const char * fmt, va_list valist);

    bool reopen_log();

    bool listen();
    
private:
    LbsConf_t & config_;
    int listen_fd_;

    static CLbsServer * sinstance;

    //for log
    int     log_fd_;
    char *  log_buf_;
    int     log_buf_size_;
    time_t  last_logfile_t_;
    pthread_mutex_t mutex_;

    std::vector<CLbsServerUnit *> server_units_;
};

#endif