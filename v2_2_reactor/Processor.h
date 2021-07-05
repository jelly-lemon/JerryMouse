#pragma once
#include "EventHandler.h"

class Processor: public EventHandler {
public:
    void handleEvents(int events) override;
};