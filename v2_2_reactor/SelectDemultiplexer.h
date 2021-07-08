#pragma once

#include <winsock.h>
#include <unordered_map>
#include "EventDemultiplexer.h"
#include "convention.h"
#include "BaseHandler.h"
#include "Acceptor.h"

class SelectDemultiplexer: public EventDemultiplexer{
private:
    fd_set read_set{};
    fd_set write_set{};
    SOCKET acceptSocket;

public:
    SelectDemultiplexer(): acceptSocket(0) {
        FD_ZERO(&read_set);
        FD_ZERO(&write_set);
    }

    bool regist(Handle handle, EventType type) {
        bool rt = false;

        switch (type) {
            case EventType::OP_READ:
                if (read_set.fd_count < FD_SETSIZE) {
                    FD_SET(handle, &read_set);
                    rt = true;
                }
                break;

            case EventType::OP_WRITE:
                if (write_set.fd_count < FD_SETSIZE) {
                    FD_SET(handle, &write_set);
                    rt = true;
                }
                break;

            case EventType::OP_ACCEPT:
                acceptSocket = handle;
                FD_SET (acceptSocket, &read_set);
                rt = true;
                break;
        }

        return rt;
    }


    int waitEvents(unordered_map<Handle, BaseHandler*> &handlers, long timeout) override {
        fd_set r_set = read_set;
        fd_set w_set = write_set;

        //
        // select 轮询
        //
        int rt;
        if (timeout == 0) {
            rt = select(0, &r_set, &w_set, NULL, NULL);
        } else {
            struct timeval selectTimeOut = {timeout, 0};
            rt = select(0, &r_set, &w_set, NULL, &selectTimeOut);
        }
        if (rt < 0) {
            err(" select failed, Err:%s\n", getErrorInfo().c_str());
            safeExit(-1);
        } else if (rt == 0) {
            //
            // select 超时返回，关闭所有连接 socket
            //
            info(" select time out, no events\n");
            for (int i = 0; i < read_set.fd_count; i++) {
                if (read_set.fd_array[i] != acceptSocket) {
                    info(" [socket %s] socket %d wait time out\n", getSocketIPPort(read_set.fd_array[i]).c_str(), read_set.fd_array[i]);
                    handleReadTimeout(read_set.fd_array[i], handlers[read_set.fd_array[i]], (Acceptor*)handlers[acceptSocket]);
                }
            }
        } else {
            //
            // 遍历每一个 socket
            //
            info(" select succeed\n");
            for (int i = 0; i < read_set.fd_count; i++) {
                if (FD_ISSET(read_set.fd_array[i], &r_set)) {
                    handlers[read_set.fd_array[i]]->handleEvent();
                }
            }

            for (int i = 0; i < write_set.fd_count; i++) {
                if (FD_ISSET(write_set.fd_array[i], &w_set)) {
                    auto *pAcceptor = (Acceptor *)handlers[acceptSocket];
                    handlers[write_set.fd_array[i]]->handleEvent(bind(&Acceptor::onConnectionClosed, pAcceptor));
                }
            }
        }

        return 0;
    }

    bool remove(Handle handle, EventType type) {
        bool rt = true;

        switch (type) {
            case EventType::OP_READ:
                FD_CLR(handle, &read_set);
                break;

            case EventType::OP_WRITE:
                FD_CLR(handle, &write_set);
                break;
        }

        return rt;
    }

    void handleReadTimeout(Handle handle, BaseHandler *pHandler, Acceptor *pAcceptor) {
        closeSocket(handle);
        auto *pReactor = Reactor::getInstance();
        pReactor->removeHandler(pHandler, EventType::OP_READ);
        pAcceptor->onConnectionClosed();
    }
};



