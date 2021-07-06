#pragma once
#include "BaseHandler.h"
#include "Event.h"
using namespace std;


/**
 * 基类
 */
class BaseHandler {

public:
    // 处理事件
    virtual void handleEvent() = 0;

    // 获取 I/O 的 Handle（句柄），实际上就是 socket
    virtual Handle getHandle() = 0;
};