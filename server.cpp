/*************************************************************************
    > File Name: server.cpp
    > Author: sudoku.huang
    > Mail: sudoku.huang@gmail.com 
    > Created Time: Thursday, December 22, 2016 AM08:58:15 CST
 ************************************************************************/

#include "server.h"
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <arpa/inet.h>
#include <strings.h>
#include <stdio.h>
#include "log.h"

#include <ulog/ulog.h>
#include <utools/ufs.h>
#include "serverunit.h"

bool get_areaisp(const std::vector<sipdb_extra_t> & db, 
    uint32 ip, std::string & area, int & isp)
{
    if (db.empty()) {
        return false;
    }

    size_t size = db.size();
    for (size_t i = 0; i < size; i++) {
        const sipdb_extra_t & one = db[i];

        if (one.mask == 32 && ip == one.ip) {
            return true;
        }

        uint32 mask = ~((1 << (32 - one.mask)) - 1);
        
        if ((one.ip & mask) == (ip & mask)) {
            return true;
        }
    }
    return false;
}

CLbsServer::CLbsServer(LbsConf_t & config, CLbsLog & log)
    : config_(config)
    , log_(log)
    , listen_fd_(-1)
    , server_units_()
    , area_db_()
    , ip_db_()
    , lbs_db_(area_db_)
{
}

CLbsServer::~CLbsServer()
{
    if (listen_fd_ > 0) {
        close(listen_fd_);
    }
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

    if (!listen()) {
        fprintf(stderr, "listen [%s:%d] failed!\n", 
            config_.server_ip.c_str(), config_.server_port);
        return false;
    }

    if (!log_.init()) {
        fprintf(stderr, "log init failed!\n"); 
        return false;
    }
    ulog(ulog_error, "\n\nprogram start here!!!\n");
    
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
