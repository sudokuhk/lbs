/*************************************************************************
    > File Name: client.h
    > Author: sudoku.huang
    > Mail: sudoku.huang@gmail.com 
    > Created Time: Thursday, December 22, 2016 AM08:58:26 CST
 ************************************************************************/

#ifndef __LBS_CLIENT_H__
#define __LBS_CLIENT_H__

#include <vector>
#include <string>
#include <map>

typedef struct LbsServerInfo
{
    std::string host;
    int         port;
} LbsServerInfo_t;

typedef struct LbsClientConf
{
    //server
    std::vector<LbsServerInfo_t> servers;
    int                         dns_timeout;
    int                         tcp_timeout;
    std::string                 area;

    //report ips
    std::map<std::string, std::string>  reportips;
    
    //log
    int                         log_level;
    std::string                 log_path;
    std::string                 log_filename;

    //sequence. service <-> id
    std::vector<std::string>    services;

    //monitor service
    std::map<std::string, int>  monitors;
} LbsClientConf_t;

class CLbsLog;

class CLbsClient
{
public:
    CLbsClient(LbsClientConf_t & config, CLbsLog & log);

    ~CLbsClient();

    bool run();
    
private:
    LbsClientConf_t & config_;
    CLbsLog & log_;
};

#endif