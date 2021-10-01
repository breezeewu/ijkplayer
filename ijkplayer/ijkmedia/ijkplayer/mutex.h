/*
 * mutex.h
 *
 * Copyright (c) 2019 sunvalley
 * Copyright (c) 2019 dawson <dawson.wu@sunvalley.com.cn>
 */

#ifndef _CACHE_BUFFER_H_
#define _CACHE_BUFFER_H_
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <pthread.h>
#include <assert.h>
#define INIT_RECURSIVE_MUTEX(pmutex) pthread_mutexattr_t attr; \
pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE); \
pthread_mutex_init(pmutex, &attr);
static pthread_mutex_t* create_mutex(int brecursive)
{
    pthread_mutex_t* pmutex = malloc(sizeof(pthread_mutex_t));
    assert(pmutex);
    if(brecursive)
    {
        pthread_mutexattr_t attr;
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
        pthread_mutex_init(pmutex, &attr);
    }
    else
    {
        pthread_mutex_init(pmutex, NULL);
    }
    return pmutex;
}

void destroy_mutex(pthread_mutex_t* pmutex)
{
    
}
#endif
