/*************************************************************************
    > File Name: cz.cpp
    > Author: sudoku.huang
    > Mail: sudoku.huang@gmail.com 
    > Created Time: Friday, December 23, 2016 AM10:16:03 CST
 ************************************************************************/

#include <stdio.h>
#include <string>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <iconv.h>    
#include <stdlib.h>

typedef unsigned char   uint8;
typedef unsigned int    uint32;
#define B2IL(b) (((b)[0] & 0xFF) | (((b)[1] << 8) & 0xFF00) | (((b)[2] << 16) & 0xFF0000) | (((b)[3] << 24) & 0xFF000000))
//#define B2IU(b) (((b)[3] & 0xFF) | (((b)[2] << 8) & 0xFF00) | (((b)[1] << 16) & 0xFF0000) | (((b)[0] << 24) & 0xFF000000))
#define GET_3BYTE(b)    \
    (((b)[0] & 0xFF) | (((b)[1] << 8) & 0xFF00) | (((b)[2] << 16) & 0xFF0000))

int code_convert(const char * from_charset, const char * to_charset,
    const char * inbuf, size_t inlen, char * outbuf, size_t outlen)    
{    
    iconv_t cd;    
    int rc;    
    char **pin = const_cast<char **>(&inbuf);    
    char **pout = &outbuf;    

    cd = iconv_open(to_charset,from_charset);    
    if (cd==0)    
        return -1;    
    memset(outbuf, 0, outlen);    
    if (iconv(cd, pin, &inlen, pout, &outlen) == -1)    
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

bool readdata(const char * filename, std::string & data)
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
    data.resize(fsize);

    int fd = open(filename, O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "open ipdb file(%s) failed!(%s)!\n", 
            filename, strerror(errno));
        return false;
    }

    if (read(fd, &data[0], fsize) != fsize) {
        fprintf(stderr, "read ipdb file(%s) failed!(%s)!\n", 
            filename, strerror(errno));
        close(fd);
        return false;
    }
    close(fd);
    return true;
}

uint32 str2ip(const char * ip) 
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

std::string ip2str(uint32 ip)
{
    char buf[64];
    snprintf(buf, 64, "%d.%d.%d.%d",
        (ip >> 24) & 0xFF,
        (ip >> 16) & 0xFF,
        (ip >> 8) & 0xFF,
        (ip >> 0) & 0xFF);

    return buf;
}

int get_param(const uint8 * pdata, uint32 off, std::string & param)
{
    //printf("param, off:%x\n", off);
    const uint8 * addr = pdata + off;
    uint8  type = *(uint8 *)(addr); //skip end ip.
    uint32 toff = 0;

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
    uint32 toff = 0;
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
}

int get(const uint8 * data, uint32 ip, std::string & country, std::string & area)
{   
    uint32 index_b  = B2IL(data);
    uint32 index_e  = B2IL(data + 4);
    //printf("index range(%x, %x)\n", index_b, index_e);
    
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
    //printf("mid:%d, off:%x, bip:%u, ip:%u\n", mid, index_b + (mid * 7), bip, ip);
    if (bip > ip) {
        mid --;
    }

    addr    = data + index_b + (mid * 7);
    bip     = B2IL(addr);
    uint32 data_off  = GET_3BYTE(addr + 4);

    const uint8 * pdata  = data + data_off;
    uint32 eip       = B2IL(pdata);

    //printf("bip:%s, eip:%s, ip:%s\n",
    //    ip2str(bip).c_str(), ip2str(eip).c_str(), ip2str(ip).c_str());

    get_location(data, data_off + 4, country, area);

    return 0;   
}

int main(int argc, char * argv[])
{
    const char * filename = "qqwry.dat";

    std::string data;
    
    if (!readdata(filename, data)) {
        fprintf(stderr, "read file failed!\n");
        exit(0);
    }

    std::string country, area;
    get((const uint8 *)data.c_str(), str2ip(argv[1]), country, area);

    char buf1[100], buf2[100];
    g2u(country.c_str(), country.size(), buf1, 100);
    g2u(area.c_str(), area.size(), buf2, 100);
    printf("%s----%s----%s\n", argv[1], buf1, buf2);

    return 0;
}
