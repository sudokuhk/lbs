/*************************************************************************
    > File Name: ipdb.cpp
    > Author: sudoku.huang
    > Mail: sudoku.huang@gmail.com 
    > Created Time: Thursday, December 22, 2016 AM09:11:41 CST
 ************************************************************************/

#include "ipdb_cz.h"
#include <stdio.h>
#include <string>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <iconv.h>    
#include <utools/ustring.h>

#define B2IL(b) \
    (((b)[0] & 0xFF) | (((b)[1] << 8) & 0xFF00) | \
    (((b)[2] << 16) & 0xFF0000) | (((b)[3] << 24) & 0xFF000000))

#define GET_3BYTE(b)    \
    (((b)[0] & 0xFF) | (((b)[1] << 8) & 0xFF00) | (((b)[2] << 16) & 0xFF0000))

int get_param(const uint8 * pdata, uint32 off, std::string & param)
{
    //printf("param, off:%x\n", off);
    const uint8 * addr = pdata + off;
    uint8  type = *(uint8 *)(addr); //skip end ip.

    switch (type) {
        case 0x02:
            get_param(pdata, GET_3BYTE(addr + 1), param);
            break;
        default:
            std::string((const char *)(pdata + off)).swap(param);
            break;
    }
    //printf("param, size:%u\n", param.size());
    return 0;
}

int get_location(const uint8 * pdata, uint32 off, 
    std::string & country, std::string & area)
{
    const uint8 *  addr = pdata + off;
    uint8  type = *(uint8 *)(addr); //skip end ip.
    //printf("location, off:%x\n", off);
    
    switch (type) {
        case 0x01:
            get_location(pdata, GET_3BYTE(addr + 1), country, area);
            break;
        case 0x02:
            get_param(pdata, GET_3BYTE(addr + 1), country);
            get_param(pdata, off + 4, area);
            break;
        default:
            get_param(pdata, off, country);
            get_param(pdata, off + country.size() + 1, area);
            break;
    }
    return 0;
}

int get(const uint8 * data, uint32 ip, std::string & country, std::string & area)
{   
    uint32 index_b  = B2IL(data);
    uint32 index_e  = B2IL(data + 4);
    
    uint32 idx_cnt  = (index_e - index_b) / 7;
    uint32 begin    = 0;
    uint32 end      = idx_cnt;
    uint32 mid;
    const uint8 * addr;
    uint32 bip;
    
    while (begin <= end) {
        mid     = (begin + end) >> 1;
        addr    = data + index_b + (mid * 7);
        bip     = B2IL(addr);
        
        if (bip == ip) {
            break;
        } else if (bip > ip) {
            end = mid - 1;
        } else {
            begin = mid + 1;
        }
    }
    if (bip > ip) {
        mid --;
    }

    addr    = data + index_b + (mid * 7);
    bip     = B2IL(addr);
    uint32 data_off  = GET_3BYTE(addr + 4);

    const uint8 * pdata  = data + data_off;
    uint32 eip       = B2IL(pdata);
    if (bip > ip || eip < ip) {
        return -1;
    }
    
    return get_location(data, data_off + 4, country, area);
}

int code_convert(const char * from_charset, const char * to_charset,
    const char * inbuf, size_t inlen, char * outbuf, size_t outlen)    
{    
    iconv_t cd;    
    char **pin = const_cast<char **>(&inbuf);    
    char **pout = &outbuf;    

    cd = iconv_open(to_charset,from_charset);    
    if (cd==0)    
        return -1;    
    memset(outbuf, 0, outlen);    
    if (iconv(cd, pin, &inlen, pout, &outlen) == (size_t) -1)    
        return -1;    
    iconv_close(cd);    
    return 0;    
}    

int u2g(const char * inbuf, int inlen, char * outbuf, size_t outlen)    
{    
    return code_convert("utf-8","gb2312", inbuf, inlen, outbuf, outlen);    
}    

int g2u(const char * inbuf, size_t inlen, char * outbuf, size_t outlen)    
{    
    return code_convert("gb2312", "utf-8", inbuf, inlen, outbuf, outlen);    
}    

std::string CIPDB_CZ::gbk2utf(const std::string & in)
{
    char buf[1024];
    if (g2u(in.c_str(), in.size(), buf, 1024) == 0) {
        return buf;
    }
    return std::string();
}

CIPDB_CZ::CIPDB_CZ()
{
}
    
CIPDB_CZ::~CIPDB_CZ()
{
}

bool CIPDB_CZ::init(const char * filename)
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
    data_.resize(fsize);

    int fd = open(filename, O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "open ipdb file(%s) failed!(%s)!\n", 
            filename, strerror(errno));
        return false;
    }

    if (read(fd, &data_[0], fsize) != fsize) {
        fprintf(stderr, "read ipdb file(%s) failed!(%s)!\n", 
            filename, strerror(errno));
        close(fd);
        return false;
    }
    close(fd);

    pdata_    = (const uint8 * )data_.data();
    index_b_  = B2IL(pdata_);    
    index_e_  = B2IL(pdata_ + 4);
    index_count_ = (index_e_ - index_b_) / en_index_size;

    if (index_e_ > fsize) {
        return false;
    }
    
    return true;
}

int CIPDB_CZ::get(uint32 ip, std::string & country, std::string & area) const
{
    return ::get(pdata_, ip, country, area);
}

int CIPDB_CZ::get(const char * ip, std::string & country, std::string & area) const
{   
    if (ip == NULL) {
        return -1;
    }

    return get(str2ip(ip), country, area);
}

