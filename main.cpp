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

int main(int argc, char * argv[])
{
    std::string appname     = get_appname(argv[0]);
    std::string version     = get_version();
    std::string workpath    = change_workpath();
    std::string config;

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

    getchar();

    return 0;
}
