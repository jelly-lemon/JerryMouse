#pragma once


#include <string>
#include "../CrossPlatform.h"
#include "../Logger.cpp"
#include "../util.cpp"
#include "HttpResponse.cpp"
#include "../SocketHelper.h"

#ifdef WIN32
#include <winsock2.h>
#else
#endif
using namespace std;


/**
 * 对服务端封装成类
 */
class HttpServer {
private:
    unsigned int connectionNumber;
    mutex numLock;
    SOCKET acceptSocket;
    int port;
    string ip;
    int backlog;

protected:
    void showAcceptSocketIPPort();






public:
    HttpServer(): connectionNumber(0), acceptSocket(0) {
#ifdef WIN32
        initWSA();
#endif
    }

    void startServer(int port = 80, string ip = "", int backlog = 65535);

    void showUsage();

    void addConnectionNumber();

    void subConnectionNumber();

    unsigned int getConnectionNumber();

    SOCKET getAcceptSocket() const;

};

SOCKET HttpServer::getAcceptSocket() const {
    return acceptSocket;
}

/**
 * 现有连接数量加 1
 */
void HttpServer::addConnectionNumber() {
    lock_guard<mutex> guarder(numLock);
    connectionNumber++;
    info(" addConnectionNumber: %d\n", connectionNumber);
}

/**
 * 现有连接数量减 1
 */
void HttpServer::subConnectionNumber() {
    lock_guard<mutex> guarder(numLock);
    connectionNumber--;
    info(" subConnectionNumber: %d\n", connectionNumber);
}



unsigned int HttpServer::getConnectionNumber() {
    lock_guard<mutex> lockGuard(numLock);
    return connectionNumber;
}

void HttpServer::showUsage() {

}

void HttpServer::startServer(int port, string ip, int backlog) {
    info(" --------- starting Server ---------\n")
    try {
        acceptSocket = createListenSocket(port, ip, backlog);
    } catch (exception &e) {
        err(" --------- startServer failed, Err: %s ---------\n", e.what());
        safeExit(-1);
    }
    info(" --------- Server started ---------\n");
}




