// 本文件包含了处理连接的子线程函数、MiniWebServer 类
#pragma once

#include <string>
#include <winsock2.h>
#include "ThreadPool.cpp"
#include "Log.cpp"

using namespace std;


/**
 * 对服务端封装成类
 */
class MiniWebServer {
private:
    ThreadPool threadPool;  // 线程池

    static void showAcceptSocketInfo(SOCKET acceptSocket);


public:
    explicit MiniWebServer(int poolSize = 30) : threadPool(ThreadPool(poolSize)) {
        initWSA();
    }

    void initWSA();

    void startServer(int port, int maxSocketNumber, string ip = "");
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

/**
 * 开启 Web Server
 *
 * @param ip 本机 ip 地址
 * @param port 监听端口
 * @param maxSocketNumber 最大监听 socket 数量
*/
void MiniWebServer::startServer(int port, int maxSocketNumber, string ip) {
    // 创建监听 socket
    SOCKADDR_IN addrSrv;
    if (ip == "" || ip == "0.0.0.0")
        addrSrv.sin_addr.S_un.S_addr = htonl(INADDR_ANY);   // INADDR_ANY 表示监听所有网卡
    else if (ip == "localhost" || ip == "127.0.0.1") {
        addrSrv.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
    } else {
        addrSrv.sin_addr.S_un.S_addr = inet_addr(ip.c_str());
    }
    addrSrv.sin_family = AF_INET;
    addrSrv.sin_port = htons(port);
    SOCKET acceptSocket = socket(AF_INET, SOCK_STREAM, 0);

    // 判断端口是否可用
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
        return;
    }

    // 等待客户端连接
    // backlog 参数表示最多建立多少个 socket 连接，只要服务端收到了客户端请求就算一个 socket
    // 当现有 socket 连接已经到了 backlog 容量，就会拒绝其余 socket 连接
    // 服务端每次 accept()，就会从队列中取出一个 socket，backlog 空位就加 1
    listen(acceptSocket, maxSocketNumber); // 开始监听请求
    showAcceptSocketInfo(acceptSocket);
    Log::record("waiting for connection...\n");
    while (1) {
        SOCKADDR_IN addrClient;
        int len = sizeof(SOCKADDR);
        // 等待连接
        SOCKET connSocket = accept(acceptSocket, (SOCKADDR *) &addrClient, &len);    // 没有客户端请求就会阻塞，有就接受
        threadPool.startThread(connSocket);
    }
}

/**
 * 初始化 socket 调用环境
 */
void MiniWebServer::initWSA() {
    WORD wVersionRequested; // WORD 就是 unsigned short，无符号短整型
    WSADATA wsaData;    // 一个结构体。这个结构被用来存储被 WSAStartup 函数调用后返回的 Windows Sockets 数据。
    wVersionRequested = MAKEWORD(1, 1); // 将两个 byte 型合并成一个 word 型,一个在高8位(b),一个在低8位(a)。整数 1 是 byte 类型吗？
    int n;

    // 初始化套接字环境
    n = WSAStartup(wVersionRequested, &wsaData);  // 即WSA(Windows Sockets Asynchronous，Windows异步套接字)的启动命令
    if (n != 0) {
        exit(-1);
    }
    // 检查是否初始化成功
    if (LOBYTE(wsaData.wVersion) != 1 || HIBYTE(wsaData.wVersion) != 1) {
        WSACleanup();   // 功能是终止 Winsock 2 DLL (Ws2_32.dll) 的使用
        exit(-1);
    }
}


