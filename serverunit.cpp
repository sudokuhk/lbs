/*************************************************************************
    > File Name: serveruint.cpp
    > Author: sudoku.huang
    > Mail: sudoku.huang@gmail.com 
    > Created Time: Monday, December 26, 2016 PM03:17:52 CST
 ************************************************************************/

#include "serverunit.h"
#include <ulog/ulog.h>

void * CLbsServerUnit::threadf(void * args)
{
    CLbsServerUnit * pobj = (CLbsServerUnit *)args;
    pobj->run();

    return NULL;
}

CLbsServerUnit::CLbsServerUnit()
    : lock_()
    , schedule_(8 * 1024, 100000, false, &lock_)
    , running_(false)
    , thread_(-1)
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
    CLbsConnection * connection = new CLbsConnection(*this, schedule_, fd, ip);
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

