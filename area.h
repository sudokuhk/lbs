/*************************************************************************
    > File Name: area.h
    > Author: sudoku.huang
    > Mail: sudoku.huang@gmail.com 
    > Created Time: Thursday, December 22, 2016 AM09:16:29 CST
 ************************************************************************/

#ifndef __LBS_AREA_H__
#define __LBS_AREA_H__

#include <string>
#include <vector>
#include <sstream>
#include <map>

enum en_isp_type
{
    en_ctc = 0,
    en_cuc,
    en_cmc,
    //en_edu,     // university && education
    en_max,
};

struct SAreaInfo
{
    int                 index;
    std::string         code;
    std::string         name;
    std::vector<int>    neigbours;

    SAreaInfo() : index(-1), code(), name(), neigbours() {}

    std::string tostring() const {
        std::stringstream sbuf;
        sbuf << index << "|"
             << name << "|"
             << code << "|";
        for (size_t i = 0; i < neigbours.size(); i++) {
            sbuf << neigbours[i] << "|";
        }
        return sbuf.str();
    }
};

struct isp_info {
    char buf[10];
    int  code;
};

class CArea
{
public:
    int get_areabyname(const std::string & areaname) const;
    int get_ispcode(const std::string & ispname) const;
    int get_areabycode(const std::string & areacode) const;
    std::string get_ispname(int isp) const;
    std::string get_areaname(int code) const;
    std::string get_areacode(int idx) const;
public:
    CArea();

    ~CArea();

    bool load(const char * filename);

    const SAreaInfo * get(int index) const;
    
    const SAreaInfo * get(std::string code) const;
private:
    std::string                 file_;
    std::vector<SAreaInfo>      area_;
    std::map<std::string, int>  area_index_;
    std::map<std::string, int>  name_index_;
    
    std::vector<std::string>    isp_;
};

#endif