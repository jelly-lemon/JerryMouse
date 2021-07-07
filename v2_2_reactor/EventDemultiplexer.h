#pragma once

#include <vector>
#include "BaseHandler.h"

using namespace std;

/**
 * I/O 多路复用 API
 */
class EventDemultiplexer {
public:
    /**
     * 等待事件
     *
     * @param handlers
     * @param timeout 超时等待秒数
     * @return
     */
    virtual int waitEvents(unordered_map<Handle, BaseHandler *> &handlers, long timeout) = 0;

    // 注册 handle
    virtual bool regist(Handle handle, EventType type) = 0;

    // 移除 handle
    virtual bool remove(Handle handle, EventType type) = 0;
};