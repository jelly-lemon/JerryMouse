#pragma once

#include <http/HttpResponse.h>

#include <utility>
#include "BaseHandler.h"

class WriteHandler: public BaseHandler {
private:
    HttpRequest httpRequest;

public:
    explicit WriteHandler(Handle handle, HttpRequest httpRequest) :
    BaseHandler(handle), httpRequest(std::move(httpRequest)) {

    }

    void handleEvent(function<void()> finishedCallback) override {
        info(" WriteHandler::handleEvent socket %d\n", sock_fd);

        HttpResponse httpResponse(httpRequest);
        httpResponse.handleRequest();

        if (finishedCallback != NULL) {
            finishedCallback();
        }

        Reactor *pReactor = Reactor::getInstance();
        pReactor->removeHandler(this, EventType::OP_WRITE);

        this->~WriteHandler();
    }
};