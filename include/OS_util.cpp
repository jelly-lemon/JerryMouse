#pragma once


#ifdef WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <sys/syscall.h>
#endif

using namespace std;

/**
 * 获取线程 ID
 */
unsigned long int getThreadID() {
    unsigned long int tid = 0;
#ifdef WIN32
    tid = GetCurrentThreadId();
#else
    tid = syscall(__NR_gettid);
#endif
    return tid;
}