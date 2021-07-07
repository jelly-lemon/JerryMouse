#pragma once

#include "BaseHandler.h"
#include "convention.h"

using namespace std;


enum EventType {
    OP_ACCEPT, OP_READ, OP_WRITE
};

/**
 * 基类
 */
class BaseHandler {
protected:
    Handle sock_fd;

public:
    BaseHandler() : sock_fd(0) {}

    // 处理事件
    virtual void handleEvent() = 0;

    Handle getHandle() {
        return sock_fd;
    }
};