/*************************************************************************
    > File Name: client.cpp
    > Author: sudoku.huang
    > Mail: sudoku.huang@gmail.com 
    > Created Time: Thursday, December 22, 2016 AM08:58:20 CST
 ************************************************************************/

#include "client.h"
#include "log.h"

CLbsClient::CLbsClient(LbsClientConf_t & config, CLbsLog & log)
    : config_(config)
    , log_(log)
{
}

CLbsClient::~CLbsClient()
{
}

bool CLbsClient::run()
{
    
}
