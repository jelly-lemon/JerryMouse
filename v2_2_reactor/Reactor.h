#pragma once

#include <ctime>
#include <list>
#include <memory>
#include "Acceptor.h"
#include "BaseHandler.h"
#include "EventDemultiplexer.h"
#include "Event.h"

using namespace std;

class Reactor {
private:
    list<BaseHandler> handlers;
    unique_ptr<EventDemultiplexer> pDemultiplexer;


public:
    Reactor(EventDemultiplexer *pDemultiplexer): pDemultiplexer(pDemultiplexer) {

    }



    //
    void handleEvents(int timeout = 0);

    void registerHandler(BaseHandler *pHandler, Event event);

    void removeHandler(BaseHandler *pHandler, Event event);
};