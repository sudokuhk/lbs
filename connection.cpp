/*************************************************************************
    > File Name: connection.cpp
    > Author: sudoku.huang
    > Mail: sudoku.huang@gmail.com 
    > Created Time: Monday, December 26, 2016 PM03:06:43 CST
 ************************************************************************/

#include "connection.h"
#include <ulog/ulog.h>
#include <utools/ustring.h>
#include <stdlib.h>
#include <string.h>
#include "lbsdb.h"

const CLbsConnection::handler_type CLbsConnection::handlers[] = {
    {"/favicon.ico",    &CLbsConnection::favicon},
    {"/tysetcpu",       &CLbsConnection::setcpu},
    {"/tyshowlive",     &CLbsConnection::dump},
    {NULL, NULL},
};

CLbsConnection::CLbsConnection(const LbsConf_t & config, CLbsMonitor & monitor, 
        uschedule & schudule, int fd, const char * ip)
    : config_(config)
    , monitor_(monitor)
    , schudule_(schudule)
    , socket_(1024, fd, &schudule_)
    , peerip_(ip)
    , http_(socket_, *this, false, false, true)
{
    socket_.set_timeout(30 * 1000); // 60s
}

CLbsConnection::~CLbsConnection()
{
}

void CLbsConnection::run()
{
    http_.run();
    monitor_.onDisconnect(socket_.socket());
    ulog(ulog_debug, "disconnect to fd:%d, (%s)!\n", 
        socket_.socket(), peerip_.c_str());

    if (socket_.good()) {
        // if connection is ok, wait 20ms.
        // if client don't close, server close it active.
        char buf;
        socket_.set_timeout(20);
        socket_.read(&buf, 1);
    }
    
    ::close(socket_.socket());
    delete this;
}

int CLbsConnection::onhttp(const uhttprequest & request,
    uhttpresponse & response, int errcode)
{
    if (errcode == uhttp::en_socket_reset) {
        ulog(ulog_debug, "socket reset by peer(%s)!\n", peerip_.c_str());
        return -1;
    }

    if (request.method() == uhttp_method_unknown) {
        ulog(ulog_error, "unknown request from (%s)!\n", peerip_.c_str());
        return -1;
    }
    
    const std::string & path = request.uri().path();
    const CLbsConnection::handler_type * h;
    int ret = 0;
    
    for (h = handlers; h->cmd != NULL; h ++) {
        if (strcmp(h->cmd, path.c_str()) == 0) {
            ulog(ulog_debug, "find handler for path:%s\n", h->cmd);
            break;
        }
    }
    
    if (h->cmd == NULL) {
        if (get(request, response, errcode) != 0) {
            ulog(ulog_error, "unknown request from(%s):%s\n", 
                peerip_.c_str(), path.c_str());
            ret = notfind(request, response, errcode);
        } else {
            ret = 0;
        }
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

int CLbsConnection::setcpu(const uhttprequest & request, 
    uhttpresponse & response, int errcode)
{
    ulog(ulog_debug, "setcpu!\n");
    #if 0 //debug
    static std::vector<std::string> services;
    if (services.empty()) {
        services.resize(10);
        services[2] = "media";
        services[9] = "publisher";
    }
    #else
    const std::vector<std::string> & services = config_.services;
    #endif    

    const uuri & uri = request.uri();
    
    std::map<std::string, std::string> limits = uri.query();

    std::string dxip = limits["dx"];
    std::string ltip = limits["lt"];
    std::string ydip = limits["yd"];
    std::string area = limits["area"];
    
    int band = atoi(limits["band"].c_str());;
    int net  = atoi(limits["net"].c_str());

    limits.erase("port");   //remove client listen port. no need!
    limits.erase("dx");
    limits.erase("lt");
    limits.erase("yd");
    limits.erase("area");
    limits.erase("band");
    limits.erase("net");
    if (net == 0 && band != 0) {
        limits["net"] = band;
    }

    if (area.empty()) {
        return -1;
    }
    
    std::map<std::string, std::string> service2port;
    typedef std::map<std::string, std::string>::iterator const_ssit;
    
    for (size_t i = 0; i < services.size(); i++) {
        if (services[i].empty()) {
            continue;
        }
        
        const_ssit it = limits.find(services[i]);
        if (it != limits.end()) {
            service2port[it->first] = it->second;
            limits.erase(it);
        }
    }

    std::map<std::string, int> limitsint;
    for (const_ssit it = limits.begin(); it != limits.end(); ++it) {
        ulog(ulog_debug, "limits, %s --> %s\n", it->first.c_str(), it->second.c_str());
        limitsint[it->first] = atoi(it->second.c_str());
    }

    uint32 dxuip = str2ip(dxip.c_str());
    uint32 ltuip = str2ip(ltip.c_str());
    uint32 yduip = str2ip(ydip.c_str());

    int flag1 = 0, flag2 = 0, flag3 = 0;
    if (dxuip != 0) {
        flag1 |= en_ctc_flag;
        if (ltuip == dxuip) {
            flag1 |= en_cuc_flag;
            ltuip = 0;
        } 
        if (yduip == dxuip) {
            flag1 |= en_cmc_flag;
            yduip = 0;
        } 
    }

    if (ltuip != 0) {
        flag2 |= en_cuc_flag;
        if (yduip == ltuip) {
            flag2 |= en_cmc_flag;
            yduip = 0;
        } 
    }

    if (yduip != 0) {
        flag3 |= en_cmc_flag;
    }
    
    for (const_ssit it = service2port.begin(); it != service2port.end(); ++ it) {
        std::vector<std::string> vport;
        split(it->second.c_str(), ',', vport);

        for (size_t i = 0; i < vport.size(); i++) {
            int port = atoi(vport[i].c_str());
            if (port == 0) {
                continue;
            }
            
            if (dxuip != 0) {
                monitor_.onNewNode(it->first, dxuip, port, flag1, area, limitsint);
            }

            if (ltuip != 0) {
                monitor_.onNewNode(it->first, ltuip, port, flag2, area, limitsint);
            }

            if (yduip != 0) {
                monitor_.onNewNode(it->first, yduip, port, flag3, area, limitsint);
            }
        }
    }

    {
        char * rdbuf = (char *)malloc(1024);
        int net, cpu, id, port;
        std::map<std::string, int> loading;
        
        while (socket_.getline(rdbuf, 1024).good()) {
            loading.clear();
            
            ulog(ulog_debug, "contine(%s):%s\n", peerip_.c_str(), rdbuf);

            if (sscanf(rdbuf, "net%d", &net) == 1) {
                loading["net"] = net;     

                for (const_ssit it = service2port.begin(); 
                    it != service2port.end(); ++ it) {
                    std::vector<std::string> vport;
                    split(it->second.c_str(), ',', vport);

                    for (size_t i = 0; i < vport.size(); i++) {
                        int port = atoi(vport[i].c_str());
                        if (port == 0) {
                            continue;
                        }
                        
                        if (dxuip != 0) {
                            monitor_.onNewNode(it->first, dxuip, port, flag1, 
                                area, limitsint);
                            monitor_.onUptNode(it->first, dxuip, port, loading);
                        }

                        if (ltuip != 0) {
                            monitor_.onNewNode(it->first, ltuip, port, flag2, 
                                area, limitsint);
                            monitor_.onUptNode(it->first, ltuip, port, loading);
                        }

                        if (yduip != 0) {
                            monitor_.onNewNode(it->first, yduip, port, flag3, 
                                area, limitsint);
                            monitor_.onUptNode(it->first, yduip, port, loading);
                        }
                    }
                }
                        
            } else if (sscanf(rdbuf, "del%d %d", &id, &port) == 2) {
                std::string service = services[id];
                if (dxuip != 0) {
                    monitor_.onDelNode(service, dxuip, port);
                }

                if (ltuip != 0) {
                    monitor_.onDelNode(service, ltuip, port);
                }

                if (yduip != 0) {
                    monitor_.onDelNode(service, yduip, port);
                }
                continue;
            } else if (sscanf(rdbuf, "cpu%d %d %d", &id, &port, &cpu) == 3) {
                loading["cpu"] = cpu;   
                std::string service = services[id];

                if (dxuip != 0) {
                    monitor_.onUptNode(service, dxuip, port, loading);
                }

                if (ltuip != 0) {
                    monitor_.onUptNode(service, ltuip, port, loading);
                }

                if (yduip != 0) {
                    monitor_.onUptNode(service, yduip, port, loading);
                }
            } else {
                continue;
            }
        }

        free(rdbuf);
    }

    //disconnect
    
        
    
    for (const_ssit it = service2port.begin(); it != service2port.end(); ++ it) {
        
        std::vector<std::string> vport;
        split(it->second.c_str(), ',', vport);

        for (size_t i = 0; i < vport.size(); i++) {
            int port = atoi(vport[i].c_str());
            if (port == 0) {
                continue;
            }
            
            if (dxuip != 0) {
                ulog(ulog_info, "service[%s] addr[%s:%d:%d] disconnect!\n",
                    it->first.c_str(), dxip.c_str(), port, flag1);
                monitor_.onDelNode(it->first, dxuip, port);
            }

            if (ltuip != 0) {
                ulog(ulog_info, "service[%s] addr[%s:%d:%d] disconnect!\n",
                    it->first.c_str(), ltip.c_str(), port, flag2);
                monitor_.onDelNode(it->first, ltuip, port);
            }

            if (yduip != 0) {
                ulog(ulog_info, "service[%s] addr[%s:%d:%d] disconnect!\n",
                    it->first.c_str(), ydip.c_str(), port, flag3);
                monitor_.onDelNode(it->first, yduip, port);
            }
        }
    }
    
    return -1;
}

int CLbsConnection::dump(const uhttprequest & request, 
    uhttpresponse & response, int errcode)
{
    std::string out;
    monitor_.onDump(out);
    response.set_content(out);

    response.set_statuscode(uhttp_status_ok);

    response.set_header(uhttpresponse::HEADER_CONTENT_TYPE, "text/html");
    return 0;
}

int CLbsConnection::get(const uhttprequest & request, 
    uhttpresponse & response, int errcode)
{
    // handle proxy ???
    std::string service;
    uint32 uip = str2ip(peerip_.c_str());
    const std::string & path = request.uri().path();

    ulog(ulog_debug, "get path:%s, prefix:%s\n", path.c_str(), config_.prefix.c_str());

    std::string match = path.substr(1); //skip /
    if (!config_.prefix.empty()) {
        std::string::size_type npos = path.find(config_.prefix);
        if (npos != 1) { //skip root path.
            return -1;
        }
        match = path.substr(npos + config_.prefix.size());
    }

    for (size_t i = 0; i < config_.services.size(); i++) {
        if (config_.services[i].empty()) {
            continue;
        }
        if (strcasecmp(match.c_str(), config_.services[i].c_str()) == 0) {
            service = config_.services[i];
            break;
        }
    }

    if (service.empty()) {
        return -1;
    }
    
    std::string out;
    monitor_.onGetNode(service, uip, out);
    response.set_content(out);

    //ulog(ulog_info, "service:[%s], client:[%s], out:[%s]\n", 
    //    service.c_str(), peerip_.c_str(), out.c_str());

    response.set_statuscode(uhttp_status_ok);

    response.set_header(uhttpresponse::HEADER_CONTENT_TYPE, "text/plain");
    return 0;
}

