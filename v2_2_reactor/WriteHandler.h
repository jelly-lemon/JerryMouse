#pragma once
#include "BaseHandler.h"

class WriteHandler: public BaseHandler {
public:
    WriteHandler(Handle sock_fd) {
        this->sock_fd = sock_fd;
    }

    void handleEvent() override {
        Reactor reactor = Reactor::getInstance();
        reactor.removeHandler(this, EventType::OP_WRITE);
    }
};