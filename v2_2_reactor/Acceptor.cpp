

#include "Acceptor.h"


/**
 * 处理连接事件
 *
 * @param event
 */
void Acceptor::handleEvent() {
    for (int i; i < spareNumber; i++) {
        SOCKET connSocket = accept(acceptSocket);
        if (connSocket != SOCKET_ERROR) {
            addConnectedSocket(connSocket);
            event.pHandler = new Handler(connSocket);
        } else {
            err(" accept failed, Err: %s\n", getErrorInfo().c_str);
        }
    }
}

void Acceptor::addConnectedSocket(SOCKET connSocket) {
    FD_SET(fds, &connSocket);
    {
        lock_guard<mutex> lockGuard(connNumberMutex);
        connNumber++;
        info(" addConnectedSocket: %d\n", connNumber);
    }
}