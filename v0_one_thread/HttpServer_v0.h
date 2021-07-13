#pragma once

#include "http/HttpResponse.h"
#include "http/HttpServer.h"

using namespace std;

class HttpServer_v0 : public HttpServer {
public:
    explicit HttpServer_v0(int port = 80, string ip = "127.0.0.1", int backlog = 65535) :
    HttpServer(port, ip, backlog) {

    }

private:
    void handleAccept() override {

        while (true) {
            sockaddr connAddr = {};
            int addrLen = sizeof(connAddr);
            SOCKET clientSocket = accept(listenSocket, &connAddr, &addrLen);
            if (clientSocket == SOCKET_ERROR) {
                err(" accept failed, Err: %s\n", getErrorInfo().c_str());
            } else {
                info("[socket %s] new socket %d\n", getSocketIPPort(clientSocket).c_str(), clientSocket);
            }
            try {
                HttpResponse response(clientSocket);
                response.handleRequest();
            } catch(exception &e) {
                err(" handleRequest failed, Err: %s\n", getErrorInfo().c_str());
            }

            closeSocket(clientSocket);
        }
    }
};


