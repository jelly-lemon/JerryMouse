#pragma once

#include <string>
#include "../include/Logger.h"
#include "../include/http/HttpServer.h"
#include "../include/ThreadPool.cpp"


using namespace std;


/**
 * 对服务端封装成类
 */
class HttpServer_v1_1 : public HttpServer{
private:
    ThreadPool<pair<SOCKET, long>> threadPool;

public:
    explicit HttpServer_v1_1(): threadPool(0)   {
#ifdef WIN32
        initWSA();
#endif
    }


    void startServer();
};




/**
 * 开启 Web Server
 *
 * @param ip 本机 ip 地址
 * @param port 监听端口
 * @param maxSocketNumber 最大监听 socket 数量
*/
void HttpServer_v1_1::startServer(int port, string ip, int backlog) {
    info(" starting Server...\n")

    //
    // 创建监听 socket
    //
    SOCKET acceptSocket = createListenSocket(port, backlog, ip);

    //
    // 接收连接
    //
    while (true) {
        sockaddr connAddr = {};
        int addrLen = sizeof(connAddr);
        SOCKET newConnSocket = accept(acceptSocket, &connAddr, &addrLen);
        info("[socket %s] new socket %d\n", getSocketIPPort(newConnSocket).c_str(), newConnSocket);
        long acceptedTime = getTickCount();
        //
        // 提交任务
        //
        threadPool.submitTask(make_pair(newConnSocket, acceptedTime));
    }
}


