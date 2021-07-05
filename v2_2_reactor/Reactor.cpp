#include "Reactor.h"

using namespace std;


/**
 * 处理所有事件
 *
 * @param ptv 超时等待时间
 */
void Reactor::handleEvents(timeval *ptv) {
    if (event is new connection) {
        //
        // 连接事件
        //
        SOCKET newSocket = accept();
        acceptor.handleEvents(newSocket);
    } else if (event is readable) {
        //
        // 可读事件
        //
        pHandler = getHandler(event);
        pHandler->handleEvents();
    }
}

/**
 *
 */
void Reactor::run() {

}
