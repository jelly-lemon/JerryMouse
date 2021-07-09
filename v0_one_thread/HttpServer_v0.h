#pragma once

#include <thread>
#include <functional>
#include "../include/http/HttpResponse.h"
#include "../include/http/HttpServer.h"

using namespace std;

class HttpServer_v0 : public HttpServer {
public:
    explicit HttpServer_v0(int port = 80, string ip = "127.0.0.1", int backlog = 65535) :
    HttpServer(port, ip, backlog) {

    }


    void startServer() {
        HttpServer::startServer();

        while (true) {
            sockaddr connAddr = {};
            int addrLen = sizeof(connAddr);
            //
            // 接收连接，启动处理线程
            //
            SOCKET newConnSocket = accept(acceptSocket, &connAddr, &addrLen);
            info("[socket %s] new socket %d\n", getSocketIPPort(newConnSocket).c_str(), newConnSocket);
            try {
                HttpResponse response(newConnSocket);
                response.handleRequest();
            } catch (exception &e) {
                info(" handleRequest failed, Err: %s\n", e.what());
            }
            closeSocket(newConnSocket);
        }
    }
};


