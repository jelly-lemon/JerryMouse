// 本文件包含了处理连接的子线程函数、MiniWebServer 类
#pragma once

#include <string>
#include <winsock2.h>
#include <iostream>
#include "ThreadPool.cpp"
#include "HttpResponse.cpp"

using namespace std;

/**
 * 处理连接需要开启的子线程函数
 */
void *t_main(void *args) {
    // 解析参数
    ThreadArgs *pThreadArgs = (ThreadArgs *) args;
    SOCKET connSocket = pThreadArgs->connSocket;
    ThreadPool *pThreadPool = pThreadArgs->pThreadPool;
    delete pThreadArgs;

    // 对客户端请求进行响应
    HttpResponse response(connSocket);
    response.handleRequest();

    // 线程池现有线程数量减 1
    pThreadPool->subCurrentNumber();

    return NULL;
}

/**
 * 对服务端封装成类
 */
class MiniWebServer {
private:
    ThreadPool threadPool;  // 线程池
public:
    explicit MiniWebServer(int poolSize = 30) : threadPool(ThreadPool(poolSize)) {
        initWSA();
    }

    void initWSA();

    void startServer(string ip, int port, int listenNumber);
};


/**
 * 开启 Web Server
 *
 * @param ip 本机 ip 地址
 * @param port 监听端口
 * @param listenNumber 最大监听 socket 数量
*/
void MiniWebServer::startServer(string ip, int port, int listenNumber) {
    // 创建监听 socket
    SOCKADDR_IN addrSrv;
    addrSrv.sin_addr.S_un.S_addr = htonl(INADDR_ANY);   // INADDR_ANY 表示监听所有网卡
    addrSrv.sin_family = AF_INET;
    addrSrv.sin_port = htons(port);
    SOCKET sockSrv = socket(AF_INET, SOCK_STREAM, 0);
    bind(sockSrv, (SOCKADDR *) &addrSrv, sizeof(SOCKADDR));
    // backlog 参数表示最多建立多少个 socket 连接，只要服务端收到了客户端请求就算一个 socket
    // 当现有 socket 连接已经到了 backlog 容量，就会拒绝其余 socket 连接
    // 服务端每次accept()，就会从队列中取出一个 socket，backlog 空位就加 1
    listen(sockSrv, listenNumber); // 监听是否有请求，非阻塞

    cout << "waiting for connection..." << endl;
    // 等待客户端连接
    while (1) {
        SOCKADDR_IN addrClient;
        int len = sizeof(SOCKADDR);
        // 等待连接
        SOCKET connSocket = accept(sockSrv, (SOCKADDR *) &addrClient, &len);    // 没有客户端请求就会阻塞，有就接受
        threadPool.startThread(t_main, connSocket);
    }
}

/**
 * 初始化 socket 调用环境
 */
void MiniWebServer::initWSA() {
    WORD wVersionRequested; // WORD 就是 unsigned short，无符号短整型
    WSADATA wsaData;    // 一个结构体。这个结构被用来存储被 WSAStartup 函数调用后返回的 Windows Sockets 数据。
    wVersionRequested = MAKEWORD(1, 1); // 将两个 byte 型合并成一个 word 型,一个在高8位(b),一个在低8位(a)。整数 1 是 byte 类型吗？
    int result;

    // 初始化套接字环境
    result = WSAStartup(wVersionRequested, &wsaData);  // 即WSA(Windows Sockets Asynchronous，Windows异步套接字)的启动命令
    if (result != 0) {
        exit(-1);
    }
    // 检查是否初始化成功
    if (LOBYTE(wsaData.wVersion) != 1 || HIBYTE(wsaData.wVersion) != 1) {
        WSACleanup();   // 功能是终止 Winsock 2 DLL (Ws2_32.dll) 的使用
        exit(-1);
    }
}
