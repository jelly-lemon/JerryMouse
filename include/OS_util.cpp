#pragma once


#ifdef WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif
#include <thread>

using namespace std;

unsigned long int getThreadID() {
    unsigned long int tid = 0;
#ifdef WIN32
    tid = GetCurrentThreadId();
#else
    tid = pthread_self();
#endif
    return tid;
}