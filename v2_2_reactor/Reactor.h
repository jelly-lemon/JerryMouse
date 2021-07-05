#pragma once

#include <ctime>
#include "EventHandler.h"
#include "Acceptor.h"
#include "Handler.h"

class Reactor {
private:
    Acceptor acceptor;
    Handler handler;

public:
    Reactor() {

    }

    void run();

    void handleEvents(timeval *ptv);

    void registerHandler(EventHandler *pHandler, int event);

    void removeHandler(EventHandler *pHandler, int event);
};