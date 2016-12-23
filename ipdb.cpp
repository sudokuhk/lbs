/*************************************************************************
    > File Name: ipdb.cpp
    > Author: sudoku.huang
    > Mail: sudoku.huang@gmail.com 
    > Created Time: Thursday, December 22, 2016 AM09:11:41 CST
 ************************************************************************/

#include "ipdb.h"
#include <stdio.h>
#include <string>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>


#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define B2IL(b) (((b)[0] & 0xFF) | (((b)[1] << 8) & 0xFF00) | (((b)[2] << 16) & 0xFF0000) | (((b)[3] << 24) & 0xFF000000))
#define B2IU(b) (((b)[3] & 0xFF) | (((b)[2] << 8) & 0xFF00) | (((b)[1] << 16) & 0xFF0000) | (((b)[0] << 24) & 0xFF000000))

CIPDB::CIPDB()
    : db_()
{
}
    
CIPDB::~CIPDB()
{
    db_.clear();
}

bool CIPDB::init(const char * filename)
{
    if (filename == NULL) {
        fprintf(stderr, "ipdb filename NULL!\n");
        return false;
    }

    if (access(filename, F_OK) != 0) {
        fprintf(stderr, "ipdb file (%s) can't access.\n", filename);
        return false;
    }

    struct stat sb;
    if (stat(filename, &sb) != 0) {
        fprintf(stderr, "ipdb file (%s) stat failed!(%s)\n", 
            filename, strerror(errno));
        return false;
    }

    off_t fsize = sb.st_size;
    db_.resize(fsize);

    int fd = open(filename, O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "open ipdb file(%s) failed!(%s)!\n", 
            filename, strerror(errno));
        return false;
    }

    if (read(fd, &db_[0], fsize) != fsize) {
        fprintf(stderr, "read ipdb file(%s) failed!(%s)!\n", 
            filename, strerror(errno));
        close(fd);
        return false;
    }
    close(fd);

    ipcount_ = fsize / en_ip_segsize;

    return true;
}

SIpInfo CIPDB::get(unsigned int ip) const
{
    SIpInfo ipinfo;
        
    unsigned int start  = 0L; //[
	unsigned int end    = ipcount_; //)
	unsigned int mid    = 0L;
	unsigned int *addr  = NULL;
    
	while(start != end) {
	
		mid = (start + end) >> 1;
		addr = (unsigned int *)(db_.data() + mid * en_ip_segsize);
        
		if(ip == *addr || end == start +1) {
			ipinfo.area = *(unsigned short*)(addr + 1);
			ipinfo.isp  = *((unsigned char*)addr + 6);
            break;
		} else if(ip > *addr) {
			start = mid;
		} else {// <
			end = mid;
		}
	}
    
    return ipinfo;
}

SIpInfo CIPDB::get(const char * ip) const
{   
    SIpInfo info;
    if (ip == NULL) {
        return info;
    }

    return get(str2ip(ip));
}

unsigned int CIPDB::str2ip(const char * ip) const
{
    unsigned int num[4];
    unsigned int ret = 0;
    int cnt = sscanf(ip, "%d.%d.%d.%d", &num[0], &num[1], &num[2], &num[3]);

    if (cnt == 4) {
        //return B2IU(ips);
        ret = ret | (num[0] << 24) | (num[1] << 16) | (num[2] << 8) | (num[3]);
    }

    return ret;
}
