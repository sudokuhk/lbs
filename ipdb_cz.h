/*************************************************************************
    > File Name: ipdb.h
    > Author: sudoku.huang
    > Mail: sudoku.huang@gmail.com 
    > Created Time: Thursday, December 22, 2016 AM09:11:38 CST
 ************************************************************************/

#ifndef __LBS_IPDB_PURE_H__
#define __LBS_IPDB_PURE_H__

#include <string>

typedef unsigned char uint8;
typedef unsigned int  uint32;

class CIPDB_CZ //chunzhen
{
public:
    //chunzhen ipdb. GBK.
    static std::string gbk2utf(const std::string & in);
    
public:
    CIPDB_CZ();
    
    ~CIPDB_CZ();

    bool init(const char * filename);

    int get(uint32 ip, std::string & country, std::string & area) const;

    int get(const char * ip, std::string & country, std::string & area) const;

private:
    uint32 str2ip(const char * ip) const;
    std::string ip2str(uint32 ip) const;
    
private:
    std::string data_;
    const uint8 * pdata_;
    uint32 index_b_;
    uint32 index_e_;
    uint32 index_count_;

    enum {
        en_index_size = 7,
    };
};

#endif