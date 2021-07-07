#pragma once

#include <mutex>
#include "BaseHandler.h"
#include "http/HttpServer.cpp"
#include "Reactor.h"
#include "ReadHandler.h"

/**
 * 处理连接请求
 */
class Acceptor : public BaseHandler {
private:
    unsigned int spareNumber;
    HttpServer httpServer;

public:
    explicit Acceptor(int port = 80, string ip = "") :
            spareNumber(FD_SETSIZE - 1), httpServer() {
        httpServer.startServer(port, ip);
        this->sock_fd = httpServer.getAcceptSocket();
    }


    /**
     * 处理连接事件
     *
     * @param event
     */
    void handleEvent() {
        //
        // 接收新连接
        //
        info(" spareNumber: %d\n", spareNumber);
        for (int i; i < spareNumber; i++) {
            sockaddr connAddr = {};
            int addrLen = sizeof(connAddr);
            SOCKET newConnSocket = accept(this->sock_fd, &connAddr, &addrLen);
            if (newConnSocket != SOCKET_ERROR) {
                info(" [ socket %s] new socket %d\n", getSocketIPPort(newConnSocket).c_str(), newConnSocket);
                httpServer.addConnectionNumber();
                spareNumber--;

                //
                // 注册监听事件
                //
                Reactor reactor = Reactor::getInstance();
                reactor.registerHandler(new ReadHandler(newConnSocket), EventType::OP_READ);
            } else {
                err(" accept failed, Err: %s\n", getErrorInfo().c_str());
            }
        }
    }

    void addSpareNumber() {
        spareNumber++;
    }

};