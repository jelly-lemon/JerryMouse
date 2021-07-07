#pragma once

#include <ctime>
#include <list>
#include <memory>
#include <mutex>
#include <unordered_map>
#include "BaseHandler.h"
#include "EventDemultiplexer.h"

using namespace std;

class Reactor {
private:
    unordered_map<Handle, BaseHandler *> handlers;
    EventDemultiplexer *pDemultiplexer;



private:
    Reactor(): pDemultiplexer(NULL) {

    }

    static Reactor *pReactor;


public:


    /**
     * 处理事件
     *
     * @param timeout
     */
    void handleEvents(int timeout = 0) {
        while (true) {
            pDemultiplexer->waitEvents(handlers, timeout);
        }
    }


    /**
     * 获取 reactor 单例对象
     */
    static Reactor &getInstance() {
        if (pReactor == NULL) {
            pReactor = new Reactor;
        }

        return *pReactor;
    }

    /**
     * 注册监听某个 handle 的某种事件
     *
     * @param handle
     * @param event
     */
    void registerHandler(BaseHandler *pHandler, EventType type) {
        handlers[pHandler->getHandle()] = pHandler;
        pDemultiplexer->regist(pHandler->getHandle(), type);
    }

    /**
     * 撤销监听某个 handle 的某种事件
     *
     * @param handle
     * @param event
     */
    void removeHandler(BaseHandler *pHandler, EventType type) {
        handlers.erase(pHandler->getHandle());
        pDemultiplexer->remove(pHandler->getHandle(), type);
    }

    void setEventDemultiplexer(EventDemultiplexer *pDemultiplexer) {
        this->pDemultiplexer = pDemultiplexer;
    }
};

Reactor *Reactor::pReactor = NULL;