/*************************************************************************
    > File Name: connection.cpp
    > Author: sudoku.huang
    > Mail: sudoku.huang@gmail.com 
    > Created Time: Monday, December 26, 2016 PM03:06:43 CST
 ************************************************************************/

#include "connection.h"
#include <ulog/ulog.h>

const CLbsConnection::handler_type CLbsConnection::handlers[] = {
    {"/favicon.ico",    &CLbsConnection::favicon},
    {NULL, NULL},
};

CLbsConnection::CLbsConnection(CLbsMonitor & monitor, uschedule & schudule, 
        int fd, const char * ip)
    : monitor_(monitor)
    , schudule_(schudule)
    , socket_(4096, fd, &schudule_)
    , peerip_(ip)
    , http_(socket_, *this, false)
{
}

CLbsConnection::~CLbsConnection()
{
}

void CLbsConnection::run()
{
    http_.run();
    monitor_.onDisconnect(socket_.socket());
    delete this;
}

int CLbsConnection::onhttp(const uhttprequest & request,
    uhttpresponse & response, int errcode)
{
    if (errcode == uhttp::en_socket_reset) {
        ulog(ulog_debug, "socket reset by peer(%s)!\n", peerip_.c_str());
        return -1;
    }
    
    const std::string path = request.uri().path();
    const CLbsConnection::handler_type * h;
    int ret = 0;
    
    for (h = handlers; h->cmd != NULL; h ++) {
        if (strcmp(h->cmd, path.c_str()) == 0) {
            ulog(ulog_debug, "find handler for path:%s\n", h->cmd);
            break;
        }
    }
    
    if (h->cmd == NULL) {
        ulog(ulog_error, "not find handler for path:%s\n", path.c_str());
        ret = notfind(request, response, errcode);
    } else {
        handler hf = h->func;
        ret = (this->*hf)(request, response, errcode);
    }
    return ret;
}

int CLbsConnection::favicon(const uhttprequest & request, 
    uhttpresponse & response, int errcode)
{
    ulog(ulog_debug, "favicon request!\n");
    response.set_statuscode(uhttp_status_ok);
    response.set_header(uhttpresponse::HEADER_CONTENT_TYPE, "text/html");
    
    return 0;
}

int CLbsConnection::notfind(const uhttprequest & request, 
    uhttpresponse & response, int errcode)
{
    ulog(ulog_debug, "notfind!\n");
    response.set_statuscode(uhttp_status_not_found);

    response.set_header(uhttpresponse::HEADER_CONTENT_TYPE, "text/html");
    response.set_content("<html><body><h1>404 Not Found!</h1></body></html>");

    return -1;
}

