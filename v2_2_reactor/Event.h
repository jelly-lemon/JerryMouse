#pragma once

#include "BaseHandler.h"
typedef int Handle;
using namespace std;

/**
 * READ: 可读
 * WRITE: 可写
 * ERROR: 出现错误
 */
enum EventType {
    READ, WRITE, ERROR
};

/**
 * 事件对象
 */
struct Event {
    EventType type;
    SOCKET socket;
};