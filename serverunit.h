/*************************************************************************
    > File Name: serveruint.h
    > Author: sudoku.huang
    > Mail: sudoku.huang@gmail.com 
    > Created Time: Monday, December 26, 2016 PM03:17:49 CST
 ************************************************************************/


#ifndef __LBS_SERVER_UNIT_H__
#define __LBS_SERVER_UNIT_H__

#include <pthread.h>
#include <unetwork/uschedule.h>
#include "connection.h"

class CLbsServerUnit
    : public CLbsConnection::CLbsMonitor
{    
public:
    static void * threadf(void * args);

private:
    class CLock : public ulock
    {
    public:
        CLock()                 {   pthread_mutex_init(&mutex_, NULL);  }
        virtual ~CLock()        {   pthread_mutex_destroy(&mutex_);     }
        virtual void lock()     {   pthread_mutex_lock(&mutex_);        }
        virtual void unlock()   {   pthread_mutex_unlock(&mutex_);      }
    private:
        pthread_mutex_t mutex_;
    };
    
public:
    CLbsServerUnit();

    ~CLbsServerUnit();

    void start();

    void stop();

    bool accept(int fd, const char * ip);

public:
     virtual void onDisconnect(int fd);
    
private:
    void run();

private:
    CLock       lock_;
    uschedule   schedule_;
    bool        running_;
    pthread_t   thread_;
};

#endif

