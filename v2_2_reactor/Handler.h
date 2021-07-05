#pragma once
#include "EventHandler.h"
#include "Processor.h"

class Handler: public EventHandler {
private:
    Processor processor;

public:
    void handleEvents(int events) override;
};