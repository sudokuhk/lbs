/*************************************************************************
    > File Name: main.cpp
    > Author: sudoku.huang
    > Mail: sudoku.huang@gmail.com 
    > Created Time: Wednesday, December 21, 2016 PM05:04:58 CST
 ************************************************************************/

#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <string>
#include <iostream>

#include "area.h"
#include "lbsdb.h"
#include "server.h"
#include <uconfig/uconfig.h>
#include <ulog/ulog.h>

#ifdef USE_IPDB_CZ
#include "ipdb_cz.h"
#else
#include "ipdb.h"
#endif

#ifndef MAJOR
#define MAJOR   0
#endif

#ifndef MINOR
#define MINOR   0
#endif

#ifndef BUILD
#define BUILD   1
#endif

const char * get_version()
{   
    #define VERSION_BUF_SIZE 30
    static char version[VERSION_BUF_SIZE] = {0};
    if (version[0] == 0) {
        snprintf(version, VERSION_BUF_SIZE, "%2d.%2d.%3d", MAJOR, MINOR, BUILD);
    }
    return version;
    #undef VERSION_BUF_SIZE
}

void show_version(const char * appname, const char * version)
{
    fprintf(stderr, "%s %s\n", appname, version); 
}

void show_usage(const char * appname)
{
    fprintf(stderr,        
"Usage: %s [-c conf]\n"
"\n"
"Mandatory arguments to long options are mandatory for short options too.\n"
"\n"
"   -c          configure file\n"
"   -v          show version\n"
"   -h          show this manual\n"
"\n", appname);

    return;
}

std::string get_appname(const char * arg0)
{
    std::string pathname(arg0);

    std::string::size_type npos = pathname.rfind("/");
    if (npos != std::string::npos) {
        return pathname.substr(npos + 1);
    }
    return pathname;
}

std::string change_workpath()
{
    pid_t pid = getpid();
    char buf[1024];
    char linkname[4096];
    
    snprintf(buf, 1024, "/proc/%d/exe", pid);

    ssize_t size = readlink(buf, linkname, 4096);
    if (size < 0) {
        fprintf(stderr, "readlink(%s) failed! reason:%s\n", buf, strerror(errno));
        exit(0);
    }

    linkname[size] = '\0';

    char * sep = strrchr(linkname, '/');
    if (sep != NULL) {
        *sep = '\0';
    } else {
        fprintf(stderr, "get workpath failed!(%s)!\n", linkname);
        exit(0);
    }

    if (chdir(linkname) != 0) {
        fprintf(stderr, "chdir %s failed!(%s)!\n", linkname, strerror(errno));
        exit(0);
    }
    
    return linkname;
}

bool load_config(const std::string & file, LbsConf_t & config)
{
    uconfig * pconfig = uconfig::create();
    if (pconfig == NULL) {
        fprintf(stderr, "create config obj failed!\n");
        return false;
    }

    bool suc = pconfig->load(file.c_str());
    if (!suc) {
        delete pconfig;
        return suc;
    }

    // server
    do {
        const char * value = NULL;
        
        config.server_ip = "0.0.0.0";
        if ((value = pconfig->get_string("server", "ip")) != NULL) {
            config.server_ip = value;
        } else {
            fprintf(stderr, "don't config host ip, use default(%s)!\n", 
                config.server_ip.c_str());
        }

        config.server_port  = pconfig->get_int("server", "port");
        if (config.server_port == 0) {
            fprintf(stderr, "don't config listen port!\n");
            suc = false;
            break;
        }
        
        config.threads = 1;
        if ((value = pconfig->get_string("server", "threads")) != NULL) {
            config.threads = atoi(value);
        } else {
            fprintf(stderr, "don't config threads, use default(%d)!\n", 
                config.threads);
        }

        config.ipdb = "";
        if ((value = pconfig->get_string("server", "ipdb")) != NULL) {
            config.ipdb = value;
        } else {
            fprintf(stderr, "don't config server::ipdb!\n");
            suc = false;
            break;
        }

        config.areadb = "";
        if ((value = pconfig->get_string("server", "areadb")) != NULL) {
            config.areadb = value;
        } else {
            fprintf(stderr, "don't config server::areadb!\n");
            suc = false;
            break;
        }
    } while (0);

     // log
    do {
        const char * value = NULL;
        
        config.log_level = ulog_error;
        if ((value = pconfig->get_string("log", "level")) != NULL) {
            config.log_level = getlevelbystring(value);
            //fprintf(stdout, "loglevel:%d %s\n", config.log_level, value);
        } else {
            fprintf(stderr, "don't config log level, use default(%d)!\n", 
                config.log_level);
        }

        config.log_path = "./log/";
        if ((value = pconfig->get_string("log", "path")) != NULL) {
            config.log_path = value;
        } else {
            fprintf(stderr, "don't config log path, use default(%s)!\n", 
                config.log_path.c_str());
        }
    } while (0);

    delete pconfig;
    
    return suc;
}

int main(int argc, char * argv[])
{
    std::string appname     = get_appname(argv[0]);
    std::string version     = get_version();
    std::string workpath    = change_workpath();
    std::string config;


    #if 0
    CArea area;
    area.load("sheng.conf");
    
    #if 0
    int idx = 0;
    const SAreaInfo * areainfo = NULL;
    while((areainfo = area.get(idx ++)) != NULL) {
        //fprintf(stderr, "%s\n", areainfo->tostring().c_str());
        //std::cerr << areainfo->tostring() << std::endl;
    }
    #endif
    #endif

#if 0
#ifdef USE_IPDB_CZ
    CIPDB_CZ ipdb;
    ipdb.init("qqwry.dat");

    std::string country, areaname;
    ipdb.get(argv[1], country, areaname);
    //printf("%s --> %s %s\n", argv[1], 
    //    CIPDB_CZ::gbk2utf(country).c_str(),
    //    CIPDB_CZ::gbk2utf(areaname).c_str());
    printf("%s  --> %d %d\n", argv[1], 
        area.get_areacode(country), 
        area.get_ispcode(areaname));
#else
    CIPDB ipdb;
    ipdb.init("ipdb.dat");

    SIpInfo ipinfo;

    ipinfo = ipdb.get(argv[1]);
    printf("%s  --> %d %d\n", argv[1], ipinfo.area, ipinfo.isp);
#endif
#endif

#if 0
    CLbsDB lbsdb;
    std::map<std::string, int> param;
    lbsdb.add("gateway", 456, 8010, 7, 20, param);
    lbsdb.add("gateway", 4567, 8010, 3, 26, param);
    lbsdb.add("media", 45689111, 8010, 1, 20, param);
    lbsdb.add("gateway", 456891111, 8010, 1, 20, param);
    lbsdb.add("gateway", 45659111, 8010, 1, 20, param);
    
    std::string str;
    lbsdb.dump(str, 1);
    std::cout << str << std::endl;


    std::cout << "\n\n\nget gateway\n";
    std::vector<node_type> nodes = lbsdb.get("gateway", 0, 20, 2);
    for (size_t i = 0; i < nodes.size(); i++) {
        std::cout << nodes[i].ipstr() << ":" << nodes[i].port << std::endl;
    }

    std::cout << "\n\n\ndel\n";
    lbsdb.remove("gateway", 456, 8010);

    std::cout << "\n\n\ndump\n";
    str.clear();
    lbsdb.dump(str, 1);
    std::cout << str << std::endl;
    //getchar();
#endif

#if 1
    if (argc == 1) {
        show_usage(appname.c_str());
        exit(0);
    }
    
    int ch;
    while((ch = getopt(argc, argv, "c:dhv")) != -1) {
        switch(ch) {
            case 'h': 
                show_usage(appname.c_str());
                exit(0);
            case 'v':
                show_version(appname.c_str(), version.c_str());
                exit(0);
            case 'c':
                std::string(optarg).swap(config);
                break;
            default:
                fprintf(stderr, "unknown arguments. (%c)", ch);
                break;
        }
    }

    if (config.empty()) {
        fprintf(stderr, "configure file empty, please check!\n");
        exit(0);
    }

    if (config.empty()) {
        //fprintf(stderr, "empty config file!\n\n");
        show_usage(appname.c_str());
        exit(0);
    }

    LbsConf_t lbsconf;
    if (!load_config(config, lbsconf)) {
        fprintf(stderr, "load configure file(%s) failed!\n\n", config.c_str());
        //show_usage(appname.c_str());
        exit(0);
    }
    lbsconf.log_filename = appname;
    lbsconf.log_filename.append(".log");

    CLbsServer server(lbsconf);
    if (!server.run()) {
        fprintf(stderr, "run error!\n");
        exit(0);
    }
#endif
    
    return 0;
}
