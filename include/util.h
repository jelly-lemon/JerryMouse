#pragma once

#include "CrossPlatform.h"
#ifdef WIN32
#include "winsock2.h"
#include "windows.h"
#elif linux
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/times.h>
#endif

#include <sstream>
#include <cstring>
#include <fcntl.h>
#include <csignal>

#include "OSHelper.cpp"
#include "Logger.h"



using namespace std;


#ifdef linux
/**
 * 使用 select 模拟毫秒级别的挂起
 *
 * @param time 毫秒
 */
void Sleep(int time) {
    struct timeval sTime;
    sTime.tv_sec = time / 1000;
    sTime.tv_usec = time % 1000;
    select(0, NULL, NULL, NULL, &sTime);
}
#endif

/**
 * 安全退出
 *
 * @param code 退出代码
 */
void safeExit(int code, string exitInfo = "") {
    info("exit: %d %s\n", code, exitInfo.c_str())

    //
    // 等待日志线程打印所有完所有消息
    //
    while (!Logger::msgQueue.isEmpty()) {
        info("waiting logger write-thread to finish msgQueue\n");
        Sleep(1000);
    }

#ifdef WIN32
    WSACleanup();
#endif
    _exit(code);
}



/**
 * 信号处理函数
 *
 * @param signal
 */
void sigHandler(int signal){
    switch (signal) {
        // Ctrl+C
        case SIGINT:
            Logger::print(&cout, " recv Ctrl+C, exiting...\n");
            safeExit(-1);
            break;
    }
}






/**
 * 获取当前时间
 */
long getCurrentTime() {
#ifdef WIN32
    static BOOL init = FALSE;
    static BOOL hires = FALSE;
    static __int64 freq = 1;
    if(!init) {
        hires = QueryPerformanceFrequency((LARGE_INTEGER*)&freq);
        if(!hires) {
            freq = 1000;
        }
        init = TRUE;
    }
    __int64 now = 0;
    if(hires) {
        QueryPerformanceCounter((LARGE_INTEGER*)&now);
    }
    else {
        now = GetTickCount();
    }

    long tickCount = (long)(1000.0f * (double)now/(double)freq);

    return tickCount;
#else
    long tickCount = (long)times( NULL);
    return tickCount;
#endif
    return 0;
}



/**
 * 获取时间差
 */
long getTimeDiff(long startTime, long endTime = 0) {
    if (endTime == 0) {
        endTime = getCurrentTime();
    }

    long timeDiff = endTime - startTime;
    if (timeDiff < 0) {
        timeDiff = 0xFFFFFFFF - startTime + endTime + 1;
    }


    return timeDiff;
}









