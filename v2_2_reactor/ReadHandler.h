#pragma once

#include <string>
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
    ReadHandler(Handle sock_fd) {
        this->sock_fd = sock_fd;
    }

    void handleEvent() override {
        //
        // 读处理
        //


        HttpResponse response(this->sock_fd);
        response.handleRequest();

        //
        // 业务处理
        //
        info("[socket %s] do something...\n", getSocketIPPort(this->sock_fd).c_str());

        //
        // 注册可写监听
        //
        Reactor reactor = Reactor::getInstance();
        reactor.registerHandler(new WriteHandler(this->sock_fd), EventType::OP_WRITE);
    }
};