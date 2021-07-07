#pragma once

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
        sprintf(t, "%s:%d", inet_ntoa(acceptAddr.sin_addr), ntohs(acceptAddr.sin_port));
        acceptIPPort = string(t);
    }

    return acceptIPPort;
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
    info(" acceptSocket: %d\n", acceptSocket);
    info(" server listen at %s\n", getAcceptIPPort(acceptSocket).c_str());


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
    if (port == 80) {
        info(" now, you can visit http://%s to browse homepage.\n", ip.c_str());
    } else {
        info(" now, you can visit http://%s:%d to browse homepage.\n", ip.c_str(), port);
    }
    info(" web_root dir is %s\n", HttpResponse::rootDir.c_str())
    info(" waiting for connection...\n");

    return acceptSocket;
}


