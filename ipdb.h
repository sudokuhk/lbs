/*************************************************************************
    > File Name: ipdb.h
    > Author: sudoku.huang
    > Mail: sudoku.huang@gmail.com 
    > Created Time: Thursday, December 22, 2016 AM09:11:38 CST
 ************************************************************************/

#ifndef __LBS_IPDB_H__
#define __LBS_IPDB_H__

#include <string>

struct SIpInfo
{
    int area;// : 16;
    int isp;// : 8;
    int reserved;// : 8;

    SIpInfo() : area(-1), isp(-1), reserved(0) {}
};

class CIPDB
{
public:
    CIPDB();
    
    ~CIPDB();

    bool init(const char * filename);

    SIpInfo get(unsigned int ip) const;

    SIpInfo get(const char * ip) const;

private:
    unsigned int str2ip(const char * ip) const;
    
private:
    std::string db_;
    int         ipcount_;

    enum {
        en_ip_segsize = 7,
    };
};

#endif