#pragma once
#include <string>
#include "Logger.h"
#include "util.h"
#ifdef WIN32
#include "winsock2.h"
#else

#endif

using namespace std;

/**
 * 获取 listenSocket 监听的 IP 和端口
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
        sprintf(t, "%s:%d", inet_ntoa(acceptAddr.sin_addr), ntohs(acceptAddr.sin_port));
        acceptIPPort = string(t);
    }

    return acceptIPPort;
}

/**
 *  将 socket 设置为非阻塞
 */
int setNonBlocking(SOCKET sockfd, bool isNonBlocking = true) {
#ifdef WIN32
    unsigned long ul = 1;
    int rt = ioctlsocket(sockfd, FIONBIO, &ul);
#else
    if (isNonBlocking) {
        int cflags = fcntl(sockfd, F_GETFL, 0);
        int rt = fcntl(sockfd, F_SETFL, cflags|O_NONBLOCK);
        if (rt == -1) {
            info(" set socket %d to NonBlocking mode failed\n", sockfd);
        } else {
            info(" set socket %d to NonBlocking mode succeed\n", sockfd);
        }

        return rt;
    }
#endif

    return -1;
}

/**
 * 创建监听 socket
 *
 * @param port
 * @param backlog
 * @param ip
 * @return
 */
SOCKET createListenSocket(int port = 80, string ip = "", int backlog = 65535) {
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
    if (ip.empty() || ip == "0.0.0.0") {
        ip = "0.0.0.0";
        addrSrv.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
    }
    else if (ip == "localhost" || ip == "127.0.0.1") {
        ip = "127.0.0.1";
        addrSrv.sin_addr.S_un.S_addr = inet_addr(ip.c_str());
    } else {
        // TODO 检查 IP 合法性
        addrSrv.sin_addr.S_un.S_addr = inet_addr(ip.c_str());
    }
    addrSrv.sin_family = AF_INET;
    // TODO 检查端口合法性
    addrSrv.sin_port = htons(port);
    SOCKET acceptSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);

    //
    // 绑定监听端口
    //
    int n = bind(acceptSocket, (SOCKADDR * ) & addrSrv, sizeof(SOCKADDR));
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
    SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    //
    // 绑定监听端口
    //
    int n = bind(listenSocket, (sockaddr *) &addrSrv, sizeof(sockaddr));
    if (n == -1) {
        err(" bind port %d failed, Err:%s\n", port, getErrorInfo().c_str())
        safeExit(-1);
    }
#endif
    info(" listenSocket: %d\n", listenSocket);
    info(" server listen at %s\n", getAcceptIPPort(listenSocket).c_str());


    //
    // 开始监听请求
    //
    //-------------------------------------------------------------
    //
    // 有两个队列：半连接（SYN_RCVD 状态）队列和全连接（ESTABLISHED 状态）队列
    // backlog 指全连接队列大小
    //
    //-------------------------------------------------------------
    listen(listenSocket, backlog);
    info(" backlog is %d\n", backlog)
    if (port == 80) {
        info(" now, you can visit http://%s to browse homepage.\n", ip.c_str());
    } else {
        info(" now, you can visit http://%s:%d to browse homepage.\n", ip.c_str(), port);
    }

    return listenSocket;
}


/**
 * 关闭 socket
 *
 * @param clientSocket
 * @return
 */
int closeSocket(SOCKET clientSocket) {
    string strIpPort = getSocketIPPort(clientSocket);
#ifdef WIN32
    int n = closesocket(clientSocket);
#else
    int n = close(clientSocket);

#endif
    if (n == SOCKET_ERROR) {
        err("[socket %s] close socket %d err, Err: %s\n", strIpPort.c_str(), clientSocket, getErrorInfo().c_str());
    } else {
        info("[socket %s] we closed socket %d.\n", strIpPort.c_str(), clientSocket);
    }

    return n;
}


    /**
     * 提前创建客户端 socket
     *
     * @return 客户端 socket
     */
SOCKET createSocket() {
    //
    // 提前创建好 socket
    //
    SOCKET newSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (newSocket == INVALID_SOCKET) {
        err(" create socket failed, Err: %s\n", getErrorInfo().c_str());
        safeExit(-1);
    } else {
        info(" create socket succeed, socket: %d\n", newSocket);
    }

    return newSocket;
}
