/*************************************************************************
    > File Name: server.h
    > Author: sudoku.huang
    > Mail: sudoku.huang@gmail.com 
    > Created Time: Thursday, December 22, 2016 AM08:58:32 CST
 ************************************************************************/

#ifndef __LBS_SERVER_H__
#define __LBS_SERVER_H__

#include <string>
#include <vector>
#include <map>

#include "lbsdb.h"
#include "area.h"
#include "ipdb_cz.h"

typedef unsigned int uint32;

typedef struct sipdb_extra
{
    uint32      ip;
    uint32      mask;
    uint32      isp;
    std::string area;
} sipdb_extra_t;

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
    int             delayclose;

    //log
    int             log_level;
    std::string     log_path;
    std::string     log_filename;

    //sequence. service <-> id
    std::vector<std::string> services;

    // extend ipdb.
    std::vector<sipdb_extra_t>  extra_ipdb;
    //std::map<uint32, std::string>   ipdb_area;
    //std::map<uint32, int>           ipdb_isp;
    
} LbsConf_t;

bool get_areaisp(const std::vector<sipdb_extra_t> & db, 
    uint32 ip, std::string & area, int & isp);
    
class CLbsServerUnit;
class CLbsLog;

class CLbsServer
{   
public:
    CLbsServer(LbsConf_t & config, CLbsLog & log);

    ~CLbsServer();
    
    bool run();

public:
    CLbsDB & lbsdb();
    const CArea & areadb() const;
    const CIPDB_CZ & ipdb() const;
    const LbsConf_t & config() const;
    
private:
    void run_forever();
    
    bool listen();
    
private:
    LbsConf_t & config_;
    CLbsLog &   log_;
    int listen_fd_;

    std::vector<CLbsServerUnit *> server_units_;

    CArea           area_db_;
    CIPDB_CZ        ip_db_;
    CLbsDB          lbs_db_;
};

#endif