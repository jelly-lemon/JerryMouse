// 本文件包含了处理连接的子线程函数、MiniWebServer 类
#pragma once

#ifndef WIN32
#error please use windows
#endif
#include <winsock2.h>
#include <string>

#include "../ThreadPool.cpp"

using namespace std;


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
    snprintf(msg, 100, "web_root dir is %s\n", IOCPHttpResponse::rootDir.c_str());
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

    unsigned long ul=1;
    ioctlsocket(acceptSocket,FIONBIO, &ul);    //设置成非阻塞模式


//    struct timeval tv;
//    tv.tv_sec = 1;
//    tv.tv_usec = 500;

    fd_set readable_fds;
    fd_set to_be_checked_fds;
    fd_set new_to_be_checked_fds;
    fd_set exceptional_fds;
    int iResult, i;

    FD_ZERO(&readable_fds);
    FD_ZERO(&to_be_checked_fds);
    FD_SET(acceptSocket, &to_be_checked_fds);
    while (1) {
        // 清空 new_to_be_checked_fds
        FD_ZERO(&new_to_be_checked_fds);

        // 监听集合中的 socket
//        Log::info(" before select, readable_fds.fd_count:%d\n", readable_fds.fd_count);

        readable_fds = to_be_checked_fds;
        exceptional_fds = to_be_checked_fds;
        int len = readable_fds.fd_count;
        Log::log("before select:");
        for (i = 0; i < len; i++) {
            printf("%d ", readable_fds.fd_array[i]);
        }
        cout << endl;
        iResult = select(0, &readable_fds, NULL, &exceptional_fds,/*&tm*/NULL);
        Log::log("after select:");
        for (i = 0; i < len; i++) {
            printf("%d ", readable_fds.fd_array[i]);
        }
        cout << endl;
        if (0 < iResult) {
//            Log::info(" after select, readable_fds.fd_count:%d\n", readable_fds.fd_count);
            // 遍历每一个 socket，检查是否可读
            for (i = 0; i < len; i++) {
                if (FD_ISSET(to_be_checked_fds.fd_array[i], &exceptional_fds)) {
                    Log::log("socket:%d err, WSAERROR:%D", to_be_checked_fds.fd_array[i], WSAGetLastError());
                    continue;
                }

                // 该 socket 是否有可读事件
                if (FD_ISSET(to_be_checked_fds.fd_array[i], &readable_fds)) {
                    Log::log(" socket:%d is ok.\n", readable_fds.fd_array[i]);
                    //如果是监听 socket，则接收连接
                    if (to_be_checked_fds.fd_array[i] == acceptSocket) {
                        sockaddr connAddr;
                        int len = sizeof(connAddr);
                        while (true) {
                            SOCKET connSocket = accept(acceptSocket, &connAddr, &len);
                            if (connSocket == INVALID_SOCKET) {
//                                Log::info(" WSAError:%d\n", WSAGetLastError());
                                break;
                            }
                            Log::log(" new socket:%d\n", connSocket);

                            // 将新 socket 放入新集合
                            ioctlsocket(connSocket,FIONBIO, &ul);    //设置成非阻塞模式
                            FD_SET(connSocket, &new_to_be_checked_fds);
                        }
                    } else {
                        // 如果是连接 socket，则表明有可读事件
                        bool rt = threadPool.submit(readable_fds.fd_array[i]);
                        if (rt == false) {
                            Log::log("submit failed, TaskQueue is full, close socket.\n");
                            closesocket(readable_fds.fd_array[i]);
                        }
                    }
                } else {
//                    Log::info(" fd:%d, acceptSocket:%d\n", readable_fds.fd_array[i], acceptSocket);
                    if (to_be_checked_fds.fd_array[i] == acceptSocket) {
                        continue;
                    }

                    Log::log(" socket:%d go to next iter.\n", to_be_checked_fds.fd_array[i]);
                    // 该 socket 没有可读事件，放入到集合中，等待下次 select
                    FD_SET(to_be_checked_fds.fd_array[i], &new_to_be_checked_fds);
                }
            }
        } else if (0 == iResult) {
            // 超时
            Log::log(" time out\n");
        } else {
            // 其它错误
            Log::log(" select WSAError:%d\n", WSAGetLastError());
        }

        // 设置下轮监听集合
        to_be_checked_fds = new_to_be_checked_fds;
        FD_SET(acceptSocket, &to_be_checked_fds);
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


