#pragma once

/**
 * I/O 多路复用 API
 */
class EventDemultiplexer {
public:
    // 等待事件
    virtual int waitEvent(list<BaseHandler> &handlers, int timeout) = 0;

    // 注册 handle
    virtual int regist(Handle handle, Event event) = 0;

    // 移除 handle
    virtual int remove(Handle handle) = 0;
};