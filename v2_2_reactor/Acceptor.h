#pragma once

#include <mutex>
#include "BaseHandler.h"
#include "http/HttpServer.h"
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
            BaseHandler(0),
            spareNumber(FD_SETSIZE - 1),
            httpServer(port, ip) {

        httpServer.startServer();
        this->sock_fd = httpServer.getAcceptSocket();
        setNonBlocking(this->sock_fd);
    }


    /**
     * 处理连接事件
     *
     * @param event
     */
    void handleEvent(function<void()> finishedCallback = NULL) override {
        //
        // 接收新连接
        //
        info(" spareNumber: %d\n", spareNumber);
        for (int i = 0; i < spareNumber; i++) {
            sockaddr connAddr = {};
            int addrLen = sizeof(connAddr);
            SOCKET newConnSocket = accept(this->sock_fd, &connAddr, &addrLen);
            if (newConnSocket != SOCKET_ERROR) {
                info("[socket %s] new socket %d\n", getSocketIPPort(newConnSocket).c_str(), newConnSocket);
                httpServer.addConnectionNumber();
                subSpareNumber();

                //
                // 注册监听事件
                //
                Reactor *pReactor = Reactor::getInstance();
                pReactor->registerHandler(new ReadHandler(newConnSocket), EventType::OP_READ);
            } else {
#ifdef WIN32
                if (getErrorCode() == WSAEWOULDBLOCK) {
                    info(" no more new connection\n");
                    break;
                }
#endif
                err(" accept failed, Err: %s\n", getErrorInfo().c_str());
            }
        }
    }

    void addSpareNumber() {
        spareNumber++;
        info(" addSpareNumber: %d\n", spareNumber);
    }

    void subSpareNumber() {
        spareNumber--;
        info(" subSpareNumber: %d\n", spareNumber);
    }

    void onConnectionClosed() {
        httpServer.subConnectionNumber();
        addSpareNumber();
    }

    unsigned int getSpareNumber() {
        return spareNumber;
    }

    bool isSpare() {
        if (spareNumber == 0) {
            return false;
        }

        return true;
    }
};