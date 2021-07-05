#pragma once


#include <string>
#include "CrossPlatform.h"
#include "Logger.cpp"
#include "util.cpp"
#include "HttpResponse.cpp"

#ifdef WIN32
#include <winsock2.h>
#else
#endif
using namespace std;


/**
 * 对服务端封装成类
 */
class WebServer {
private:
    unsigned int connectionNumber;

    static mutex numLock;


protected:
    static void showAcceptSocketIPPort(SOCKET acceptSocket);

    SOCKET createListenSocket(int port, int backlog, string &ip);




public:
    WebServer(): connectionNumber(0) {

    }

    virtual void startServer(int port, string ip, int backlog) = 0;

    virtual void showUsage();

    void addConnectionNumber();

    void subConnectionNumber();

    int getConnectionNumber();


};


mutex WebServer::numLock;



/**
 * 现有连接数量加 1
 */
void WebServer::addConnectionNumber() {
    lock_guard<mutex> guarder(numLock);
    connectionNumber++;
    info(" addConnectionNumber: %d\n", connectionNumber);
}

/**
 * 现有连接数量减 1
 */
void WebServer::subConnectionNumber() {
    lock_guard<mutex> guarder(numLock);
    connectionNumber--;
    info(" subConnectionNumber: %d\n", connectionNumber);
}

/**
 * 打印 acceptSocket 监听的 IP 和端口
 */
void WebServer::showAcceptSocketIPPort(SOCKET acceptSocket) {
    sockaddr_in socketAddr = {};
#ifdef WIN32
    int len = sizeof(socketAddr);
#else
    socklen_t len = sizeof(socketAddr);
#endif
    int rt = getsockname(acceptSocket, (struct sockaddr *) &socketAddr, &len);
    // TODO 检查 rt 合法性
    string listenIpPort = string(inet_ntoa(socketAddr.sin_addr)) + ":" + to_string(ntohs(socketAddr.sin_port));
    info(" server listen at %s\n", listenIpPort.c_str())
}

/**
 * 创建监听 socket
 *
 * @param port
 * @param backlog
 * @param ip
 * @return
 */
SOCKET WebServer::createListenSocket(int port, int backlog, string &ip) {
    //
    // 创建监听 socket
    //
    // ---------------------------------------------------
    //
    // INADDR_ANY 表示监听所有网卡，也就是本机所有 IP 地址
    //
    // ---------------------------------------------------
#ifdef WIN32
    SOCKADDR_IN addrSrv;
    if (ip.empty() || ip == "0.0.0.0")
        addrSrv.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
    else if (ip == "localhost" || ip == "127.0.0.1") {
        addrSrv.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
    } else {
        // TODO 检查 IP 合法性
        addrSrv.sin_addr.S_un.S_addr = inet_addr(ip.c_str());
    }
    addrSrv.sin_family = AF_INET;
    // TODO 检查端口合法性
    addrSrv.sin_port = htons(port);
    SOCKET acceptSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
    info(" acceptSocket: %d\n", acceptSocket);

    //
    // 绑定监听端口
    //
    int n = bind(acceptSocket, (SOCKADDR *) &addrSrv, sizeof(SOCKADDR));
    if (n == SOCKET_ERROR) {
        err(" bind port %d failed, Err:%s\n", port, getErrorInfo().c_str())
        safeExit(-1);
    }
#else
    sockaddr_in addrSrv;
    if (ip.empty() || ip == "0.0.0.0")
        addrSrv.sin_addr.s_addr = htonl(INADDR_ANY);
    else if (ip == "localhost" || ip == "127.0.0.1") {
        addrSrv.sin_addr.s_addr = inet_addr("127.0.0.1");
    } else {
        // TODO 检查 IP 合法性
        addrSrv.sin_addr.s_addr = inet_addr(ip.c_str());
    }
    addrSrv.sin_family = AF_INET;
    // TODO 检查端口合法性
    addrSrv.sin_port = htons(port);
    SOCKET acceptSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    //
    // 绑定监听端口
    //
    int n = bind(acceptSocket, (sockaddr *) &addrSrv, sizeof(sockaddr));
    if (n == SOCKET_ERROR) {
        err(" bind port %d failed, Err:%s\n", port, getErrorInfo().c_str())
        safeExit(-1);
    }
#endif


    //
    // 开始监听请求
    //
    //-------------------------------------------------------------
    //
    // 有两个队列：半连接（SYN_RCVD 状态）队列和全连接（ESTABLISHED 状态）队列
    // backlog 指全连接队列大小
    //
    //-------------------------------------------------------------
    listen(acceptSocket, backlog);
    info(" backlog is %d\n", backlog)
    showAcceptSocketIPPort(acceptSocket);
    if (port == 80) {
        info(" now, you can visit http://%s to browse homepage.\n", ip.c_str());
    } else {
        info(" now, you can visit http://%s:%d to browse homepage.\n", ip.c_str(), port);
    }
    info(" web_root dir is %s\n", HttpResponse::rootDir.c_str())
    info(" waiting for connection...\n");

    return acceptSocket;
}

int WebServer::getConnectionNumber() {
    lock_guard<mutex> lockGuard(numLock);
    return connectionNumber;
}

void WebServer::showUsage() {

}




