#pragma once
#include <vector>
#include "BaseHandler.h"
#include "convention.h"
#include <functional>

using namespace std;


enum EventType {
    OP_ACCEPT, OP_READ, OP_WRITE
};

const vector<string> EventTypeStr = {"OP_ACCEPT", "OP_READ", "OP_WRITE"};


/**
 * 基类
 */
class BaseHandler {
protected:
    Handle sock_fd;

public:
    BaseHandler(Handle handle) : sock_fd(handle) {

    }

    // 处理事件
    virtual void handleEvent(function<void()> finishedCallback = NULL) = 0;


    Handle getHandle() {
        return sock_fd;
    }
};