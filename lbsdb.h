/*************************************************************************
    > File Name: lbsdb.h
    > Author: sudoku.huang
    > Mail: sudoku.huang@gmail.com 
    > Created Time: Friday, December 23, 2016 PM04:17:00 CST
 ************************************************************************/

#ifndef __LBS_DB_H__
#define __LBS_DB_H__

#include <map>
#include <set>
#include <vector>
#include <string>
#include <stdio.h>

typedef unsigned char uint8;
typedef unsigned int  uint32;
typedef unsigned long uint64;

#define DEF_GET_COUNT   2

enum  
{   
    en_ctc_off  = 0,
    en_cuc_off  = 1,
    en_cmc_off  = 2,

    en_ctc_flag = 1 << en_ctc_off,
    en_cuc_flag = 1 << en_cuc_off,
    en_cmc_flag = 1 << en_cmc_off,
};

struct sipinfo
{
    uint32  ip;
    int     port;
    int     area;
    int     ispflag;
    
    std::map<std::string, int>  limit;
    std::map<std::string, int>  loading;

    sipinfo() : ip(0), port(0), area(0), ispflag(0) {}

    friend bool operator<(const sipinfo & lhs, const sipinfo & rhs)
    {
        return lhs.ip > rhs.ip ? true : lhs.port - rhs.port;
    }

    friend bool operator==(const sipinfo & lhs, const sipinfo & rhs)
    {
        return lhs.ip == rhs.ip && lhs.port == rhs.port;
    }

    uint64 key() const {
        uint64 k = ip;
        k = (k << 32) | port;
        return k;
    }

    static uint64 key(uint32 ip, int port) {
        uint64 k = ip;
        k = (k << 32) | port;
        return k;
    }

    std::string ipstr() const {
        char buf[64];
        snprintf(buf, 64, "%d.%d.%d.%d",
            (ip >> 24) & 0xFF,
            (ip >> 16) & 0xFF,
            (ip >> 8) & 0xFF,
            (ip >> 0) & 0xFF);

        return buf;
    }

    std::string portstr() const {
        char buf[64];
         snprintf(buf, 64, "%d", port);

        return buf;
    }

    int get_limit(const char * key) const {
        std::map<std::string, int>::const_iterator it = limit.find(key);
        if (it != limit.end()) {
            return it->second;
        }

        return 0;
    }

    int get_loading(const char * key) const {
        std::map<std::string, int>::const_iterator it = loading.find(key);
        if (it != loading.end()) {
            return it->second;
        }

        return 0;
    }
};

typedef sipinfo                             node_type;
typedef sipinfo *                           pnode_type;
typedef std::map<uint64, node_type>         node_map;       //second unused.
typedef std::set<pnode_type>                pnode_array;
typedef std::map<std::string, pnode_array>  service_map;    // name->ipinfo.
typedef std::vector<service_map>            area_table;     //hash table.
typedef std::vector<area_table>             isp_table;
typedef pthread_mutex_t                     lock_type;

class CArea;

class CLbsDB
{
public:
    enum {
        en_strategy_cpu,
        en_strategy_net,
    };
    
    enum {
        en_dump_plain = 0,
        en_dump_html,
    };
public:
    CLbsDB(const CArea & areadb);

    ~CLbsDB();

    int add(const std::string & service, uint32 ip, int port, int ispflag, 
        int area, const std::map<std::string, int> & limit);

    int update(const std::string & service, uint32 ip, int port,
        const std::map<std::string, int> & params);

    int remove(const std::string & service, uint32 ip, int port);

    std::vector<node_type> get(const std::string & service, 
        int isp, int area, int count = DEF_GET_COUNT, 
        int strategy = en_strategy_cpu);

    void dump(std::string & out, int format = en_dump_plain);
    
private:
    void initlock();
    void lock();
    void unlock();
    void destroylock();

    void addone(const std::string & service, node_type * pnode);
    void delone(const std::string & service, node_type * pnode);

    void dump_plain(std::string & out);
    void dump_html(std::string & out);

private:
    node_map        node_map_;
    isp_table       isp_table_;
    lock_type       lock_;
    const CArea &   areadb_;
};

#endif