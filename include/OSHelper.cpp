#pragma once


#ifdef WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <sys/syscall.h>
#endif

using namespace std;



/**
 * 模拟事务
 *
 * @param milliseconds 毫秒
 */
void cpuRun(int milliseconds) {
#ifdef WIN32
    DWORD startTime = GetTickCount();
    while (1) {
        if (GetTickCount() - startTime > milliseconds) {
            break;
        }
    }
#else
    // TODO
#endif
}

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


/**
 * 获取 CPU 逻辑核心数
 */
int getCPULogicCoresNumber() {
    int n = 0;
#ifdef WIN32
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    n = sysInfo.dwNumberOfProcessors;
#else

#endif
    return n;
}