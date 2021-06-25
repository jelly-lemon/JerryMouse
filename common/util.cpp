#pragma once


#include <sstream>
#include <cstring>
#include "Logger.cpp"

#ifdef linux
#include <unistd.h>
typedef int SOCKET;
#elif WIN32

#include "winsock2.h"

#endif

using namespace std;


/**
 *  将 socket 设置为非阻塞
 */
int setNonBlocking(SOCKET sockfd) {
    unsigned long ul = 1;
#ifdef linux
    int rt = fcntl(sockfd, FIONBIO, &ul);
#else
    int rt = ioctlsocket(sockfd, FIONBIO, &ul);
#endif
    return rt;
}


/**
 * 获取错误信息
 */
string getErrorInfo() {

#ifdef linux
    string err_info(strerror(errno));
#else
    int err_code = WSAGetLastError();
    string err_info;
    switch (err_code) {
        case WSAEADDRINUSE:
            err_info += "port is in use, can't bind";
            break;
        case WSAENOTSOCK:
            err_info += "Socket operation on nonsocket";
            break;
        case WSAETIMEDOUT:
            err_info += "recv timeout";
            break;
        default:
            err_info += to_string(err_code);
            break;
    }
#endif

    return err_info;
}

#ifdef linux
/**
 * 使用 select 模拟毫秒级别的挂起
 *
 * @param time 毫秒
 */
void Sleep(int time) {
    struct timeval sTime;
    sTime.tv_sec = 0;
    sTime.tv_usec = time * 1000;
    select(0, NULL, NULL, NULL, &sTime);
}
#endif

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
 * 安全退出
 *
 * @param code 退出代码
 */
void safeExit(int code) {
    //
    // 等待日志线程打印所有完所有消息
    //
    while (!Logger::msgQueue.isEmpty()) {
        Logger::print(&cout, "waiting logger write-thread to finish\n");
        Sleep(100);
    }

#ifdef WIN32
    WSACleanup();
#endif

    exit(code);
}

/**
 * 获取 CPU 逻辑核心数
 */
int getLogicCoresNumber() {
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
 * 获取 acceptSocket 监听的 IP 和端口
 */
string getAcceptIPPort(SOCKET &acceptSocket) {
    sockaddr_in acceptAddr;

#ifdef linux
    socklen_t len = sizeof(acceptAddr);
#else
    int len = sizeof(acceptAddr);
#endif

    string acceptIPPort;
    if (getsockname(acceptSocket, (struct sockaddr *) &acceptAddr, &len) == 0) {
        char t[100];
        sprintf(t, "%s:%d\n", inet_ntoa(acceptAddr.sin_addr), ntohs(acceptAddr.sin_port));
        acceptIPPort = string(t);
    }

    return acceptIPPort;
}


/**
 * 获取客户端 IP 和端口号
 */
string getClientIPPort(SOCKET &connSocket) {
    string clientIPport;
    sockaddr_in peerAddr;

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