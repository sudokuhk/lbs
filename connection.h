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

class CLbsConnection
    : public utask
    , public uhttphandler
{
public:
    class CLbsMonitor {
    public:
        virtual ~CLbsMonitor() {}

        virtual void onDisconnect(int fd) = 0;
    };

public:
    CLbsConnection(CLbsMonitor & monitor, uschedule & schudule, 
        int fd, const char * ip);

    virtual ~CLbsConnection();
    
    virtual void run();

    virtual int onhttp(const uhttprequest & request,
        uhttpresponse & response, int errcode);

private:
    int notfind(const uhttprequest & request, uhttpresponse & response, int );
    int favicon(const uhttprequest & request, uhttpresponse & response, int );
private:
    typedef int (CLbsConnection::*handler)(const uhttprequest &, 
        uhttpresponse &, int);
    struct handler_type {
        const char *    cmd;
        handler         func;
    };
    
    CLbsMonitor & monitor_;
    uschedule & schudule_;
    utcpsocket  socket_;
    std::string peerip_;   
    uhttp       http_;

    const static handler_type handlers[];
};

#endif
