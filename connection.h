/*************************************************************************
    > File Name: connection.h
    > Author: sudoku.huang
    > Mail: sudoku.huang@gmail.com 
    > Created Time: Monday, December 26, 2016 PM03:06:40 CST
 ************************************************************************/

#ifndef __LBS_CONNECTION_H__
#define __LBS_CONNECTION_H__

#include <uhttp/uhttp.h>
#include <unetwork/uschedule.h>
#include <unetwork/utcpsocket.h>
#include "server.h"

typedef unsigned int uint32;

class CLbsConnection
    : public utask
    , public uhttphandler
{
public:
    class CLbsMonitor {
    public:
        virtual ~CLbsMonitor() {}

        virtual void onDisconnect(int fd) = 0;
        virtual void onNewNode(const std::string & service, uint32 ip, int port, 
            int isp, const std::string & area, 
            const std::map<std::string, int> & limits) = 0;
        virtual void onDelNode(const std::string & service, uint32 ip, int port) = 0;
        virtual void onGetNode(const std::string & service, uint32 ip, 
            std::string & out) = 0;
        virtual void onUptNode(const std::string & service, uint32 ip, int port,
            const std::map<std::string, int> & loadings) = 0;
        virtual void onDump(std::string & out) = 0;
    };

public:
    CLbsConnection(const LbsConf_t & config, CLbsMonitor & monitor, 
        uschedule & schudule, int fd, const char * ip);

    virtual ~CLbsConnection();
    
    virtual void run();

    virtual int onhttp(const uhttprequest & request,
        uhttpresponse & response, int errcode);

private:
    int notfind(const uhttprequest & request, uhttpresponse & response, int );
    int favicon(const uhttprequest & request, uhttpresponse & response, int );
    int setcpu(const uhttprequest & request, uhttpresponse & response, int);
    int dump(const uhttprequest & request, uhttpresponse & response, int);
    int get(const uhttprequest & request, uhttpresponse & response, int);
private:
    typedef int (CLbsConnection::*handler)(const uhttprequest &, 
        uhttpresponse &, int);
    struct handler_type {
        const char *    cmd;
        handler         func;
    };

    const LbsConf_t & config_;
    CLbsMonitor & monitor_;
    uschedule & schudule_;
    utcpsocket  socket_;
    std::string peerip_;   
    uhttp       http_;

    const static handler_type handlers[];
};

#endif
