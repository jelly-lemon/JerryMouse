#pragma once

#include "CrossPlatform.h"
#ifdef WIN32
#include "winsock2.h"
#include "windows.h"
#elif linux
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

#include <sstream>
#include <cstring>
#include <fcntl.h>
#include <csignal>
#include "OS_util.cpp"
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
 * 获取错误信息
 *
 * 官方文档：
 * https://docs.microsoft.com/en-us/windows/win32/winsock/windows-sockets-error-codes-2
 */
string getErrorInfo() {
#ifdef linux
    string err_info = to_string(errno) + ", " + strerror(errno);
#else
    int err_code = WSAGetLastError();
    string err_info;
    switch (err_code) {
        case WSAEADDRINUSE:
            err_info += "WSAEADDRINUSE, port is in use, can't bind";
            break;
        case WSAENOTSOCK:
            err_info += "WSAENOTSOCK, Socket operation on nonsocket";
            break;
        case WSAENOTCONN:
            err_info += "WSAENOTCONN, Socket is not connected";
            break;
        case WSAEINVAL:
            err_info += "WSAEINVAL, Invalid argument";
            break;
        case WSAETIMEDOUT:
            err_info += "WSAETIMEDOUT, Connection timed out";
            break;
        case WSAEWOULDBLOCK:
            err_info += "WSAEWOULDBLOCK, Resource temporarily unavailable";
            break;
        case WSANOTINITIALISED:
            err_info += "WSANOTINITIALISED, not WSAStartup";
            break;
        case WSA_IO_PENDING:
            err_info += "WSA_IO_PENDING, Overlapped operations will complete later";
            break;
        case WSA_OPERATION_ABORTED:
            err_info += "WSA_OPERATION_ABORTED, Overlapped operation aborted";
            break;
        case WSA_INVALID_HANDLE:
            err_info += "WSA_INVALID_HANDLE, Specified event object handle is invalid";
            break;
        case WSAEFAULT:
            err_info += "WSAEFAULT, maybe the length of the buffer is too small";
            break;
        default:
            err_info += to_string(err_code);
            break;
    }
#endif

    return err_info;
}

/**
 * 获取 socket 错误码
 */
int getErrorCode() {
#ifdef linux
    int errorCode = errno;
#else
    int errorCode = WSAGetLastError();
#endif

    return errorCode;
}



/**
 * 读取文件
 */
string getFile(const string &URL) {
    string filePath = URL;
    ifstream file(filePath, ios::in | ios::binary);     // 二进制模式读取
    if (!file) {
        string msg = URL + " file not exists";
        throw invalid_argument(msg);
    } else {
        ostringstream fileContent;
        fileContent << file.rdbuf();
        return fileContent.str();
    }
}

/**
 * 根据 url 获取请问的文件类型，如 png
 *
 * @param url 请求 url
 * @return 文件类型
 */
string getFileType(string url) {
    int p = url.find_last_of(".");
    if (p != -1)
        return url.substr(p + 1);
    else
        throw runtime_error("getFileType failed, url:" + url);
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
 * 获取 CPU 逻辑核心数
 */
int getCPULogicCoresNumber() {
    int n = 0;
#ifdef linux

#else
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    n = sysInfo.dwNumberOfProcessors;
#endif

    return n;
}




/**
 * 获取客户端 IP 和端口号
 */
string getSocketIPPort(SOCKET connSocket) {
    string clientIPport;
    sockaddr_in peerAddr = {};

#ifdef linux
    socklen_t len = sizeof(peerAddr);
#else
    int len = sizeof(peerAddr);
#endif

    if (getpeername(connSocket, (struct sockaddr *) &peerAddr, &len) == 0) {
        //
        // socket 存活时才能获取成功
        //
        char info[50];
        sprintf(info, "%s:%d", inet_ntoa(peerAddr.sin_addr), ntohs(peerAddr.sin_port));
        clientIPport = string(info);
    } else {
        clientIPport = "";
    }

    return clientIPport;
}

/**
 * 获取 CPU tick count
 */
long getTickCount() {
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

#endif
    return 0;
}



/**
 * 获取时间差
 */
long getTimeDiff(long startTime, long endTime = 0) {
    if (endTime == 0) {
        endTime = getTickCount();
    }
    long timeDiff = endTime - startTime;
    if (timeDiff < 0) {
        timeDiff = 0xFFFFFFFF - startTime + endTime + 1;
    }

    return timeDiff;
}


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

#endif
}

#ifdef WIN32

/**
 * 初始化 socket 调用环境
 */
void initWSA(int a = 2, int b = 2) {
    // -----------------------------
    //
    // WORD 就是 unsigned short，无符号短整型
    // WSADATA 这个结构体被用来存储被 WSAStartup 函数调用后返回的 Windows Sockets 数据。
    // MAKEWORD 将两个 byte 型合并成一个 word 型,一个在高8位(b),一个在低8位(a)
    //
    // -----------------------------
    WORD wVersionRequested;
    WSADATA wsaData;
    wVersionRequested = MAKEWORD(a, b); //

    do {
        //
        // 初始化套接字环境
        //
        int n = WSAStartup(wVersionRequested, &wsaData);  // 即WSA(Windows Sockets Asynchronous，Windows异步套接字)的启动命令
        if (n != 0) {
            break;
        }

        //
        // 检查是否初始化成功
        //
        if (LOBYTE(wsaData.wVersion) != a || HIBYTE(wsaData.wVersion) != b) {
            break;
        }
        return;
    } while (0);

    //
    // 若初始化失败
    //
    err("WSAStartup failed, Err:%s\n", getErrorInfo().c_str())
    WSACleanup();   // 功能是终止 Winsock 2 DLL (Ws2_32.dll) 的使用
    safeExit(-1);
}

#endif


/**
 * 获取本机 IP
 */
string getLocalIP() {
    string myIP("127.0.0.1");

#ifdef WIN32
    do {
        initWSA();

        char local[255] = {0};
        gethostname(local, sizeof(local));
        hostent* ph = gethostbyname(local);
        if (ph == NULL) {
            err("gethostbyname failed\n");
            break;
        }

        // FIXME 获取所有IP
        in_addr addr;
        memcpy(&addr, ph->h_addr_list[0], sizeof(in_addr)); // 这里仅获取第一个ip
        myIP = inet_ntoa(addr);
    } while (0);
#else

#endif

    return myIP;
}

