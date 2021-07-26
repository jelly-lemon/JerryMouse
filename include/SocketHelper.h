#pragma once

/**
 * socket 相关处理函数
 */

#include <string>
#include "Logger.h"
#include "util.h"
#ifdef WIN32
#include "winsock2.h"
#else

#endif

using namespace std;


string getErrorInfo();

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
    SOCKET listenSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);

    //
    // 绑定监听端口
    //
    int n = bind(listenSocket, (SOCKADDR * ) & addrSrv, sizeof(SOCKADDR));
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
