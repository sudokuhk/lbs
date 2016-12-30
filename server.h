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

#include "lbsdb.h"
#include "area.h"
#include "ipdb_cz.h"

typedef unsigned int uint32;

typedef struct SLbsConf
{
    //server
    std::string     server_ip;
    int             server_port;
    int             threads;
    int             deamon;
    std::string     areadb;
    std::string     ipdb;
    std::string     prefix;
    int             getipcnt;
    std::string     defarea;

    //log
    int             log_level;
    std::string     log_path;
    std::string     log_filename;

    //sequence. service <-> id
    std::vector<std::string> services;

    // extend ipdb.
    std::map<uint32, std::string>   ipdb_area;
    std::map<uint32, int>           ipdb_isp;
    
} LbsConf_t;

class CLbsServerUnit;

class CLbsServer
{   
public:
    CLbsServer(LbsConf_t & config);

    ~CLbsServer();
    
    bool run();

    static void log_hook(int level, const char * fmt, va_list valist); 

public:
    CLbsDB & lbsdb();
    const CArea & areadb() const;
    const CIPDB_CZ & ipdb() const;
    const LbsConf_t & config() const;
    
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
    pthread_mutex_t log_mutex_;

    std::vector<CLbsServerUnit *> server_units_;

    CArea           area_db_;
    CIPDB_CZ        ip_db_;
    CLbsDB          lbs_db_;
};

#endif