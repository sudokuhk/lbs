/*************************************************************************
    > File Name: lbsdb.cpp
    > Author: sudoku.huang
    > Mail: sudoku.huang@gmail.com 
    > Created Time: Friday, December 23, 2016 PM04:17:04 CST
 ************************************************************************/

#include "lbsdb.h"

#include <stdio.h>
#include "area.h"

template <typename T>
void autoexpand(T & t, int newsize)
{
    size_t oldsize = t.size();
    if ((int)oldsize < newsize) {
        t.resize(newsize);
    }
}

template <typename T>
int getN(const T & t, int n, std::vector<node_type> & out)
{
    typename T::const_iterator it = t.begin();
    int cnt = 0;
    
    while (n > 0 && it != t.end()) {

        std::map<std::string, int>::const_iterator it1, it2;
        const struct sipinfo & ip = **it;
        bool valid = true;
        
        for (it1 = ip.loading.begin(); it1 != ip.loading.end(); ++ it1) {
            const std::string & key = it1->first;
            int loading = it1->second;

            it2 = ip.limit.find(key);
            if (it2 != ip.limit.end()) {
                int limit = it2->second;
                if (limit != 0 && loading >= limit) {
                    valid = false;
                    break;
                }
            }
        }

        if (valid) {
            out.push_back(**it);
            n --;
            cnt ++;
        }

        ++ it;
    }

    return cnt;
}

#if 0
class CCpuStrategy
{
public:
    bool operator()(const sipinfo & lhs, const sipinfo & rhs) {
        int cpu1 = lhs.get_loading("cpu");
        int cpu2 = rhs.get_loading("cpu");

        if (cpu1 != cpu2) {
            return cpu1 < cpu2;
        } else {
            return lhs.key() < rhs.key();
        }
    }
};

class CNetStrategy
{
public:
    bool operator()(const sipinfo & lhs, const sipinfo & rhs) {
        int cpu1 = lhs.get_loading("net");
        int cpu2 = rhs.get_loading("net");

        if (cpu1 != cpu2) {
            return cpu1 < cpu2;
        } else {
            return lhs.key() < rhs.key();
        }
    }
};
#else
bool cpu_strategy(const sipinfo * plhs, const sipinfo * prhs) 
{
    const sipinfo & lhs = *plhs;
    const sipinfo & rhs = *prhs;
    
    int cpu1 = lhs.get_loading("cpu");
    int cpu2 = rhs.get_loading("cpu");

    if (cpu1 != cpu2) {
        return cpu1 < cpu2;
    } else {
        return lhs.key() < rhs.key();
    }
}

bool net_strategy(const sipinfo * plhs, const sipinfo * prhs) 
{
    const sipinfo & lhs = *plhs;
    const sipinfo & rhs = *prhs;
    
    int cpu1 = lhs.get_loading("net");
    int cpu2 = rhs.get_loading("net");

    if (cpu1 != cpu2) {
        return cpu1 < cpu2;
    } else {
        return lhs.key() < rhs.key();
    }
}

class CCpuStrategy
{
public:
    bool operator()(sipinfo * const & plhs, sipinfo * const & prhs) {

        const sipinfo & lhs = *plhs;
        const sipinfo & rhs = *prhs;
    
        int cpu1 = lhs.get_loading("cpu");
        int cpu2 = rhs.get_loading("cpu");

        if (cpu1 != cpu2) {
            return cpu1 < cpu2;
        } else {
            return lhs.key() < rhs.key();
        }
    }
};

class CNetStrategy
{
public:
    bool operator()(sipinfo * const & plhs, sipinfo * const & prhs) {
        const sipinfo & lhs = *plhs;
        const sipinfo & rhs = *prhs;
        
        int cpu1 = lhs.get_loading("net");
        int cpu2 = rhs.get_loading("net");

        if (cpu1 != cpu2) {
            return cpu1 < cpu2;
        } else {
            return lhs.key() < rhs.key();
        }
    }
};

#endif

std::string itoa(int n) 
{
    char buf[15];
    snprintf(buf, 15, "%d", n);
    return buf;
}

CLbsDB::CLbsDB(const CArea & areadb)
    : node_map_()
    , isp_table_()
    , areadb_(areadb)
{
    initlock();
}

CLbsDB::~CLbsDB()
{
    destroylock();
}

void CLbsDB::addone(const std::string & service, node_type * pnode)
{
    int isp = pnode->ispflag;
    int idx = -1;

    while (isp != 0) {

        int bit = isp & 0x01;
        isp = isp >> 1;
        idx ++;
        
        if (bit == 0) {
            continue;
        }
        
        autoexpand(isp_table_, idx + 1);

        area_table & area = isp_table_[idx];
        autoexpand(area, pnode->area + 1);

        service_map & smap = area[pnode->area];

        pnode_array & parray = smap[service];
        
        parray.insert(pnode);
    }
}

void CLbsDB::delone(const std::string & service, node_type * pnode)
{
    int isp = pnode->ispflag;
    int idx = -1;

    while (isp != 0) {

        int bit = isp & 0x01;
        isp = isp >> 1;
        idx ++;
        
        if (bit == 0) {
            continue;
        }
        
        autoexpand(isp_table_, idx + 1);

        area_table & area = isp_table_[idx];
        autoexpand(area, pnode->area + 1);

        service_map & smap = area[pnode->area];

        pnode_array & parray = smap[service];

        pnode_array::iterator it = parray.find(pnode);
        if (it != parray.end()) {
            parray.erase(pnode);
        }
    }
}

int CLbsDB::add(const std::string & service, uint32 ip, int port, 
    int ispflag, int area, const std::map<std::string, int> & limit)
{
    if (ip == 0 || port == 0 || area < 0) {
        return -1;
    }
    
    node_type node;
    node.ip         = ip;
    node.port       = port;
    node.ispflag    = ispflag;
    node.area       = area;
    node.limit      = limit;

    lock();
    node_type & existsone = node_map_[node.key()];
    if (existsone.ispflag == ispflag && existsone.area == area) {
        unlock();
        return 1;
    } else {
        if (existsone.ip != 0) {
            delone(service, &existsone);
        }
        node_map_[node.key()] = node;
        addone(service, &node_map_[node.key()]);
    }
    unlock();
    return 0;
}

int CLbsDB::update(const std::string & service, uint32 ip, int port, 
    const std::map<std::string, int> & params)
{
    if (ip == 0 || port == 0) {
        return -1;
    }
    
    uint64 key = node_type::key(ip, port);

    lock();
    node_map::iterator it = node_map_.find(key);
    if (it != node_map_.end()) {
        node_type & node = it->second;
        
        std::map<std::string, int>::const_iterator pit = params.begin();
        while (pit != params.end()) {
            node.loading[pit->first] = pit->second;
            ++ pit;
        }
    }
    unlock();
    
    return 0;
}

int CLbsDB::remove(const std::string & service, uint32 ip, int port)
{
    if (ip == 0 || port == 0) {
        return -1;
    }

    int ret = -1;
    uint64 key = node_type::key(ip, port);
    
    lock();
    node_map::iterator it = node_map_.find(key);
    if (it != node_map_.end()) {
        node_type & node = it->second;
        delone(service, &node);
        node_map_.erase(it);
        ret = 0;
    }
    unlock();
    return ret;
}

std::vector<node_type> CLbsDB::get(const std::string & service, 
    int isp, int area, int count, int strategy)
{
    std::vector<node_type> out;

    std::set<pnode_type, CCpuStrategy> cpuset;
    std::set<pnode_type, CNetStrategy> netset;
    
    lock();
    if (isp < (int)isp_table_.size()) {
        area_table & areatb = isp_table_[isp];
        if ((int)areatb.size() > area) {
            service_map & smap = areatb[area];
            
            if (!smap.empty()) {
                service_map::const_iterator sit = smap.find(service);
                if (sit != smap.end()) {
                    const pnode_array & parray = sit->second;

                    switch (strategy) {
                        case en_strategy_net:
                            netset.insert(parray.begin(), parray.end());
                            count -= getN(cpuset, count, out);
                            break;
                        case en_strategy_cpu:
                        default:
                            cpuset.insert(parray.begin(), parray.end());
                            count -= getN(cpuset, count, out);
                            break;
                    }
                }
            }
        }
    }
    unlock();
    
    return out;
}

void CLbsDB::dump(std::string & out, int format)
{
    out.clear();
    
    lock();
    typedef void (CLbsDB::*fdump)(std::string&);
    fdump f = &CLbsDB::dump_plain;

    switch (format) {
        case en_dump_html:
            f = &CLbsDB::dump_html;
            break;
        default:
            f = &CLbsDB::dump_plain;
            break;
    }
    (this->*f)(out);
    unlock();
}

void CLbsDB::dump_plain(std::string & out)
{
    for (size_t isp = 0; isp < isp_table_.size(); isp++) {
        area_table & area_ = isp_table_[isp];
        if (area_.empty()) {
            continue;
        }
        out.append(areadb_.get_ispname(isp)).append("\n");

        for (size_t area = 0; area < area_.size(); area ++) {
            service_map & smap = area_[area];
            if (smap.empty()) {
                continue;
            }

            out.append("\t").append(areadb_.get_areacode(area))
                .append(areadb_.get_areaname(area)).append("\n");

            service_map::const_iterator it;
            for (it = smap.begin(); it != smap.end(); ++it) {
                const pnode_array & parray = it->second;
                if (parray.empty()) {
                    continue;
                }
                out.append("\t\tservice:").append(it->first).append("\n");

                pnode_array::const_iterator nit;
                for (nit = parray.begin(); nit != parray.end(); ++nit) {
                    const node_type & node = *(*nit);
                    out.append("\t\t\tnode-->").append(node.ipstr()).append(":")
                        .append(itoa(node.port)).append("\n");
                }
            }
        }
    }
}

void CLbsDB::dump_html(std::string & out)
{
    out.append("<HTML><BODY><br><br>");
    //out.append("<style>td{border:1px solid black;padding-right:16px}</style><table>");
    out.append("<table border=\"1\" cellspacing=\"0\">");
    
    for (size_t isp = 0; isp < isp_table_.size(); isp++) {
        area_table & area_ = isp_table_[isp];
        if (area_.empty()) {
            continue;
        }

        out += "<tr><td colspan=\"100\" align=\"center\">" 
            + areadb_.get_ispname(isp) + "</td></tr>";

        for (size_t area = 0; area < area_.size(); area ++) {
            service_map & smap = area_[area];
            if (smap.empty()) {
                continue;
            }

            std::string tmp;
            int count = 0;
            
            service_map::const_iterator it;
            for (it = smap.begin(); it != smap.end(); ++it) {
                const pnode_array & parray = it->second;
                if (parray.empty()) {
                    continue;
                }

                count ++;
                tmp += "<tr><td rowspan=\"" + itoa(parray.size() + 1)
                    + "\">" + it->first + "</td></tr>";
                
                pnode_array::const_iterator nit;
                for (nit = parray.begin(); nit != parray.end(); ++nit) {
                    const node_type & node = *(*nit);

                    tmp += "<tr>";
                    tmp += "<td>" + node.ipstr() + ":" + itoa(node.port) + "</td>";

                    typedef std::map<std::string, int>::const_iterator constit;
                    for (constit it = node.loading.begin(); 
                        it != node.loading.end(); ++ it) {

                        int limit   = 0;
                        constit limitit = node.limit.find(it->first);
                        if (limitit != node.limit.end()) {
                            limit = limitit->second;
                        } 
                        
                        int loading = it->second;
                        tmp += "<td>" + it->first + ":" + itoa(loading)
                            + "/" + itoa(limit) + "</td>";
                    }
                    
                    tmp += "</tr>";
                    
                    count ++;
                }
            }

            out += "<tr><td rowspan=\"" + itoa(count + 1)
                    + "\">" + areadb_.get_areacode(area)
                + areadb_.get_areaname(area) + "</td></tr>";
            out += tmp;
        }
    }
    out.append("</table></BODY></HTML>");
}

void CLbsDB::initlock()
{
    pthread_mutex_init(&lock_, NULL);
}

void CLbsDB::lock()
{
    pthread_mutex_lock(&lock_);
}

void CLbsDB::unlock()
{
    pthread_mutex_unlock(&lock_);
}

void CLbsDB::destroylock()
{
    pthread_mutex_destroy(&lock_);
}

