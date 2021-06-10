// 本文件包含了处理连接的子线程函数、MiniWebServer 类
#pragma once

#include <string>
#include <winsock2.h>
#include "ThreadPool.cpp"
#include "Log.cpp"

using namespace std;


SOCKET g_connSocket[FD_SETSIZE];    // 存放

/**
 * 对服务端封装成类
 */
class MiniWebServer {
private:
    ThreadPool threadPool;  // 线程池对象

    static void showAcceptSocketInfo(SOCKET acceptSocket);


public:
    explicit MiniWebServer(int poolSize = 30): threadPool(poolSize) {
        initWSA();
    }

    void initWSA();

    void startServer(int port, int maxSocketNumber, string ip = "");

    static SOCKET createListenSocket(int port, int maxSocketNumber, string ip);
};


/**
 * 打印 acceptSocket 监听的 IP 和端口
 */
void MiniWebServer::showAcceptSocketInfo(SOCKET acceptSocket) {
    struct sockaddr_in socketAddr;
    int len = sizeof(socketAddr);
    getsockname(acceptSocket, (struct sockaddr *) &socketAddr, &len);
    char msg[100];
    sprintf(msg, "server listen at %s:%d\n", inet_ntoa(socketAddr.sin_addr), ntohs(socketAddr.sin_port));
    Log::record(msg);
}

SOCKET MiniWebServer::createListenSocket(int port, int maxSocketNumber, string ip) {
    // 创建监听 socket
    SOCKADDR_IN addrSrv;
    if (ip.empty() || ip == "0.0.0.0")
        addrSrv.sin_addr.S_un.S_addr = htonl(INADDR_ANY);   // INADDR_ANY 表示监听所有网卡，也就是本机所有 IP 地址
    else if (ip == "localhost" || ip == "127.0.0.1") {
        addrSrv.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
    } else {
        addrSrv.sin_addr.S_un.S_addr = inet_addr(ip.c_str());
    }
    addrSrv.sin_family = AF_INET;
    addrSrv.sin_port = htons(port);
    SOCKET acceptSocket = socket(AF_INET, SOCK_STREAM, 0);

    // 监听指定端口
    int n;
    n = bind(acceptSocket, (SOCKADDR *) &addrSrv, sizeof(SOCKADDR));
    if (n == SOCKET_ERROR) {
        int error_code = WSAGetLastError();
        char msg[101];
        if (error_code == WSAEADDRINUSE) {
            snprintf(msg, 100, "port %d is in use, can't bind\n", port);
        } else {
            snprintf(msg, 100, "can't bind socket at %s:%d, WSA error code:%d\n", ip.c_str(), port, error_code);
        }
        Log::record(msg);
        WSACleanup();
        exit(-1);
    }

    // 半连接（SYN_RCVD状态）队列大小和全连接（ESTABLISHED状态）队列大小
    // backlog 指全连接队列大小
    listen(acceptSocket, maxSocketNumber); // 开始监听请求

    char msg[101] = {'\0'};
    snprintf(msg, 100, "max accept socket number is %d\n", maxSocketNumber);
    Log::record(msg);
    showAcceptSocketInfo(acceptSocket);
    if (port == 80) {
        snprintf(msg, 100, "now, you can visit http://localhost to browse homepage.\n");
    } else {
        snprintf(msg, 100, "now, you can visit http://localhost:%d to browse homepage.\n", port);
    }
    Log::record(msg);
    snprintf(msg, 100, "root dir is %s\n", HttpResponse::rootDir.c_str());
    Log::record(msg);
    Log::record("waiting for connection...\n");

    return acceptSocket;
}


/**
 * 开启 Web Server
 *
 * @param ip 本机 ip 地址
 * @param port 监听端口
 * @param maxSocketNumber 最大监听 socket 数量
*/
void MiniWebServer::startServer(int port, int maxSocketNumber, string ip) {
    SOCKET acceptSocket = createListenSocket(port, maxSocketNumber, ip);

    fd_set fd;
    FD_ZERO(&fd);
    FD_SET(acceptSocket, &fd);

    int iResult, i;
    while (1) {
        fd_set fdOld = fd;
        iResult = select(0, &fdOld, NULL, NULL,/*&tm*/NULL);
        if (0 < iResult) {
            for (i = 0; i < fd.fd_count; i++) {
                if (FD_ISSET(fd.fd_array[i], &fdOld)) {
                    //如果socket是服务器，则接收连接
                    if (fd.fd_array[i] == acceptSocket) {
                        sockaddr connAddr;
                        int len = sizeof(connAddr);
                        SOCKET connSocket = accept(acceptSocket, &connAddr, &len);
                        FD_SET(connSocket, &fd);
                    } else {
                        threadPool.submit(fd.fd_array[i]);
                    }
                }
            }
        } else if (0 == iResult) {
            continue;
        } else {
            continue;
        }
    }

}


/**
 * 初始化 socket 调用环境
 */
void MiniWebServer::initWSA() {
    WORD wVersionRequested; // WORD 就是 unsigned short，无符号短整型
    WSADATA wsaData;    // 一个结构体。这个结构被用来存储被 WSAStartup 函数调用后返回的 Windows Sockets 数据。
    wVersionRequested = MAKEWORD(1, 1); // 将两个 byte 型合并成一个 word 型,一个在高8位(b),一个在低8位(a)。整数 1 是 byte 类型吗？

    // 初始化套接字环境
    int n = WSAStartup(wVersionRequested, &wsaData);  // 即WSA(Windows Sockets Asynchronous，Windows异步套接字)的启动命令
    if (n != 0) {
        int err = WSAGetLastError();
        char msg[101] = {'\0'};
        snprintf(msg, 100, "WSAStartup failed. err:%d\n", err);
        Log::record(msg);
        exit(-1);
    }

    // 检查是否初始化成功
    if (LOBYTE(wsaData.wVersion) != 1 || HIBYTE(wsaData.wVersion) != 1) {
        WSACleanup();   // 功能是终止 Winsock 2 DLL (Ws2_32.dll) 的使用
        int err = WSAGetLastError();
        char msg[101] = {'\0'};
        snprintf(msg, 100, "WSAStartup failed. err:%d\n", err);
        Log::record(msg);
        exit(-1);
    }
}


