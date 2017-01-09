/*************************************************************************
    > File Name: serveruint.cpp
    > Author: sudoku.huang
    > Mail: sudoku.huang@gmail.com 
    > Created Time: Monday, December 26, 2016 PM03:17:52 CST
 ************************************************************************/

#include "serverunit.h"
#include <ulog/ulog.h>
#include <utools/ustring.h>
#include "server.h"

void * CLbsServerUnit::threadf(void * args)
{
    CLbsServerUnit * pobj = (CLbsServerUnit *)args;
    pobj->run();

    return NULL;
}

CLbsServerUnit::CLbsServerUnit(CLbsServer & server)
    : lock_()
    , schedule_(8 * 1024, 100000, false, &lock_)
    , running_(false)
    , thread_(-1)
    , server_(server)
{
}

CLbsServerUnit::~CLbsServerUnit()
{
}

void CLbsServerUnit::start()
{
    pthread_create(&thread_, NULL, CLbsServerUnit::threadf, this);
}

void CLbsServerUnit::stop()
{
    schedule_.stop();
}

bool CLbsServerUnit::accept(int fd, const char * ip)
{
    CLbsConnection * connection = new CLbsConnection(server_.config(),
        *this, schedule_, fd, ip);
    schedule_.add_task(connection);
    
    return true;
}

void CLbsServerUnit::run()
{
    schedule_.run();
}

void CLbsServerUnit::onDisconnect(int fd)
{
}

void CLbsServerUnit::onNewNode(const std::string & service, uint32 ip, int port, 
            int isp, const std::string & area, 
            const std::map<std::string, int> & limits)
{
    CLbsDB & lbsdb = server_.lbsdb();
    const CArea & areadb = server_.areadb();

    const int areacode    = areadb.get_areabycode(area);
    if (areacode != -1) {
        if (lbsdb.add(service, ip, port, isp, areacode, limits) == 0) {
            ulog(ulog_info, "add service:%s, [%s:%d:%d][%s]\n",
                service.c_str(), ip2str(ip).c_str(), port, isp, area.c_str());
        } else {
            ulog(ulog_debug, "exists node, service:%s, [%s:%d:%d][%s]\n",
                service.c_str(), ip2str(ip).c_str(), port, isp, area.c_str());
        }
    } else {
        ulog(ulog_error, "unkown area(%s), service:%s, [%s:%d:%d]\n",
            area.c_str(), service.c_str(), ip2str(ip).c_str(), port, isp);
    }

    //std::string out;
    //lbsdb.dump(out, 1);
    //fprintf(stdout, "lbsdb:\n%s\n", out.c_str());
}

void CLbsServerUnit::onDelNode(const std::string & service, uint32 ip, int port)
{
    //ulog(ulog_debug, "del node, service:%s, [%s:%d]\n",
    //    service.c_str(), ip2str(ip).c_str(), port);

    CLbsDB & lbsdb = server_.lbsdb();
    if (lbsdb.remove(service, ip, port) == 0) {
        ulog(ulog_info, "del service:%s, [%s:%d]\n",
            service.c_str(), ip2str(ip).c_str(), port);
    }
}

void CLbsServerUnit::onGetNode(const std::string & service, uint32 ip, 
        std::string & out)
{
          CLbsDB &    lbsdb  = server_.lbsdb();
    const CArea &     areadb = server_.areadb();
    const CIPDB_CZ &  ipdb   = server_.ipdb();
    const LbsConf_t & config = server_.config();

    std::string area, isp;
    int iarea, iisp;

    if (get_areaisp(config.extra_ipdb, ip, area, iisp)) {
        iarea = areadb.get_areabycode(area);
        //iisp  = iisp;
        ulog(ulog_info, "get ip from extra db: ip:%s, area:%d, isp:%d\n", 
            ip2str(ip).c_str(), iarea, iisp);
    } else {
        ipdb.get(ip, area, isp);
        iarea = areadb.get_areabyname(area);
        iisp  = areadb.get_ispcode(isp);
    }

    ulog(ulog_debug, "onGetNode: ip:%s, area:%s, isp:%s\n", 
        ip2str(ip).c_str(), 
        CIPDB_CZ::gbk2utf(areadb.get_areaname(iarea)).c_str(), 
        CIPDB_CZ::gbk2utf(areadb.get_ispname(iisp)).c_str());
    if (iarea < 0) {
        iarea = areadb.get_areabycode(config.defarea); //guangdong
        if (iarea == -1) {
            ulog(ulog_error, "no default area!\n"); 
            return ;
        }
        
        ulog(ulog_info, "ip:%s, country:%s, area:%s, use default:%s\n", 
            ip2str(ip).c_str(), 
            CIPDB_CZ::gbk2utf(area).c_str(), 
            CIPDB_CZ::gbk2utf(isp).c_str(), 
            CIPDB_CZ::gbk2utf(areadb.get_areaname(iarea)).c_str());
    }

    int count = config.getipcnt;

    std::vector<node_type> outv = lbsdb.get(service, iisp, iarea, count);
    count -= outv.size();

    const SAreaInfo * info = areadb.get(iarea);
    if (info != NULL) {
        std::vector<node_type> out1;
        const std::vector<int> & neigbours = info->neigbours;

        int countin = 0;
        if (outv.size() > 0) {
            countin = 1;
            count   = 1;
        }
        
        for (size_t i = 0; i < neigbours.size(); i++) {
            if (neigbours[i] == iarea) {
                continue;
            }
            
            out1 = lbsdb.get(service, iisp, neigbours[i], count);
            outv.insert(outv.end(), out1.begin(), out1.end());
            count -= out1.size();

            if (out1.size() > 0) {
                countin ++;
            } else {
                continue;
            }

            if (countin > 1) {
                break;
            } else {
                count = 1;
            }
        }
    }

    for (size_t i = 0; i < outv.size(); i++) {
        out.append(outv[i].ipstr().c_str()).append(":")
            .append(outv[i].portstr().c_str()).append(",");
    }

    ulog(ulog_info, "service:[%s], client:[%s][%s:%s], out:[%s]\n", 
        service.c_str(), ip2str(ip).c_str(), 
        CIPDB_CZ::gbk2utf(areadb.get_areaname(iarea)).c_str(), 
        CIPDB_CZ::gbk2utf(areadb.get_ispname(iisp)).c_str(), 
        out.c_str());
}

void CLbsServerUnit::onUptNode(const std::string & service, uint32 ip, int port,
    const std::map<std::string, int> & loadings)
{
    ulog(ulog_debug, "update node, service:%s, [%s:%d]\n", 
        service.c_str(), ip2str(ip).c_str(), port);

    #if 0
    typedef std::map<std::string, int>::const_iterator const_siit;
    for (const_siit it = loadings.begin(); it != loadings.end(); ++it) {
        ulog(ulog_debug, "loading: [%s] = [%d]\n", it->first.c_str(), it->second);
    }
    #endif

    server_.lbsdb().update(service, ip, port, loadings);
}

void CLbsServerUnit::onDump(std::string & out)
{
    server_.lbsdb().dump(out, CLbsDB::en_dump_html);
}
