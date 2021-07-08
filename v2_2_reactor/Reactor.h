#pragma once

#include <ctime>
#include <list>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <Logger.h>
#include "BaseHandler.h"
#include "EventDemultiplexer.h"

using namespace std;

class Reactor {
private:
    unordered_map<Handle, BaseHandler *> handlers;
    EventDemultiplexer *pEventDemultiplexer;



private:
    Reactor(): pEventDemultiplexer(NULL) {

    }

    static Reactor *pReactor;


public:


    /**
     * 处理事件
     *
     * @param timeout 超时等待秒数
     */
    void handleEvents(int timeout = 0) {
        info(" waitEvents timeout: %d secs\n", timeout);
        while (true) {
            pEventDemultiplexer->waitEvents(handlers, timeout);
        }
    }


    /**
     * 获取 reactor 单例对象
     */
    static Reactor *getInstance() {
        if (pReactor == NULL) {
            pReactor = new Reactor;
        }

        return pReactor;
    }

    /**
     * 注册监听某个 handle 的某种事件
     *
     * @param handle
     * @param event
     */
    void registerHandler(BaseHandler *pHandler, EventType type) {
        handlers[pHandler->getHandle()] = pHandler;
        pEventDemultiplexer->regist(pHandler->getHandle(), type);
        info(" registerHandler: %d, %s\n", pHandler->getHandle(), EventTypeStr[type].c_str());
    }

    /**
     * 撤销监听某个 handle 的某种事件
     *
     * @param handle
     * @param event
     */
    void removeHandler(BaseHandler *pHandler, EventType type) {
        info(" removeHandler: %d, %s\n", pHandler->getHandle(), EventTypeStr[type].c_str());
        pEventDemultiplexer->remove(pHandler->getHandle(), type);
        handlers.erase(pHandler->getHandle());
    }

    void setEventDemultiplexer(EventDemultiplexer *pDemultiplexer) {
        pEventDemultiplexer = pDemultiplexer;
    }
};

Reactor *Reactor::pReactor = NULL;