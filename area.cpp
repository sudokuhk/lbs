/*************************************************************************
    > File Name: area.cpp
    > Author: sudoku.huang
    > Mail: sudoku.huang@gmail.com 
    > Created Time: Thursday, December 22, 2016 AM09:16:36 CST
 ************************************************************************/

#include "area.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fstream>

#include <utools/ustring.h>

//gbk code.
const unsigned char dianxin[]    = {0xB5, 0xE7, 0xD0, 0xC6};
const unsigned char liantong[]   = {0xC1, 0xAA, 0xCD, 0xA8};
const unsigned char yidong[]     = {0xD2, 0xC6, 0xB6, 0xAF};

CArea::CArea()
    : file_()
    , area_()
{
    isp_.resize(en_max);
    std::string((const char *)dianxin, 4).swap(isp_[en_ctc]);
    std::string((const char *)liantong, 4).swap(isp_[en_cuc]);
    std::string((const char *)yidong, 4).swap(isp_[en_cmc]);
}

CArea::~CArea()
{
}

int CArea::get_areacode(const std::string & areaname)
{
    for (size_t i = 0; i < area_.size(); i++) {
        if (areaname.find(area_[i].name) != std::string::npos) {
            return area_[i].index;
        }
    }
    return -1;
}

int CArea::get_ispcode(const std::string & ispname)
{
    if (ispname.size() < 4) {
        return en_ctc;
    }

    for (size_t i = 0; i < en_max; i++) {
        if (ispname.find(isp_[i]) != std::string::npos) {
            return i;
        }
    }

    return en_ctc;
}

bool CArea::load(const char * filename)
{
    if (filename == NULL) {
        fprintf(stderr, "area filename NULL!\n");
        return false;
    }

    if (access(filename, F_OK) != 0) {
        fprintf(stderr, "area file (%s) can't access.\n", filename);
        return false;
    }

    std::string(filename).swap(file_);

    const int linesize  = 4096;
    char * const rbuf   = (char *)malloc(linesize * sizeof(char));
    bool ret            = true;
    std::streamsize rdn = 0;
    int line = 0;
    
    std::ifstream in(file_.c_str());
    if (in.is_open()) {
        while (true) {
            in.getline(rbuf, linesize);
            
            if (in.eof()) {
                //printf("end of file!\n");
                break;
            }

            if (!in.good()) {
                ret = false;
                break;
            }

            line ++;
            rdn = in.gcount();
            set(rbuf + rdn, '\0');
            
            const char * end   = rbuf + rdn;
            const char * begin = rbuf;
            trimleft(begin, end);
            if (begin <= end && *begin == '#') {
                continue;
            }

            std::vector<std::string> out;
            split(rbuf, ',', out);

            size_t cnt = out.size();
            if (cnt < 3) {
                fprintf(stderr, "line:%d error!.\n", line);
                ret = false;
                break;
            }

            SAreaInfo areainfo;
            areainfo.index  = atoi(out[0].c_str());
            areainfo.name.swap(out[1]);
            areainfo.code.swap(out[2]);
            
            for (size_t i = 3; i < cnt; i++) {
                areainfo.neigbours.push_back(atoi(out[i].c_str()));
            }

            if (areainfo.index < 0) {
                fprintf(stderr, "line:%d invalid index..!.\n", line);
                ret = false;
                break;
            }

            if ((int)area_.size() < areainfo.index + 1) {
                area_.resize(areainfo.index + 1);
            }
            area_[areainfo.index]       = areainfo;
            area_index_[areainfo.code]  = areainfo.index;
            name_index_[areainfo.name]  = areainfo.index;
        }
    }

    free(rbuf);
    if (in.is_open()) {
        in.close();
    }

    return ret;
}

const SAreaInfo * CArea::get(int index) const
{
    if (index < 0 || index >= (int)area_.size()) {
        return NULL;
    }
    return &area_[index];
}

const SAreaInfo * CArea::get(std::string code) const
{
    std::map<std::string, int>::const_iterator it = area_index_.find(code);
    if (it == area_index_.end()) {
        return NULL;
    }
    return get(it->second);
}

