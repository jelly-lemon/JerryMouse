#pragma once

#include <string>
#include "http/HttpRequest.h"
#include "BaseHandler.h"
#include "Reactor.h"
#include "WriteHandler.h"


using namespace std;

/**
 *
 */
class ReadHandler : public BaseHandler {
private:


public:
    ReadHandler(Handle handle): BaseHandler(handle) {
    }

    void handleEvent(function<void()> finishedCallback) override {
        info(" ReadHandler::handleEvent socket %d\n", sock_fd);

        try {
            //
            // 读处理
            //
            HttpRequest httpRequest(sock_fd);

            //
            // 业务处理
            //
            info("[socket %s] do something...\n", getSocketIPPort(this->sock_fd).c_str());

            //
            // 注册可写监听
            //
            Reactor *pReactor = Reactor::getInstance();
            pReactor->removeHandler(this, EventType::OP_READ);
            pReactor->registerHandler(new WriteHandler(sock_fd, httpRequest), EventType::OP_WRITE);

            if (finishedCallback != NULL) {
                finishedCallback();
            }
            this->~ReadHandler();
        } catch (exception &e) {
            err(" get HttpRequest failed, Err: %s\n", e.what());
            handleError();
            return;
        }
    }

    void handleError() {
        Reactor *pReactor = Reactor::getInstance();
        pReactor->removeHandler(this, EventType::OP_READ);
        closeSocket(sock_fd);
    }
};