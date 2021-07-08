#pragma once


#include <string>
#include "../CrossPlatform.h"
#include "../Logger.h"
#include "../util.h"
#include "../SocketHelper.h"
#include "FileHelper.h"

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
    int port;
    string ip;
    int backlog;



protected:
    SOCKET acceptSocket;


public:
    explicit HttpServer(int port = 80, string ip = "", int backlog = 65535):
    connectionNumber(0), acceptSocket(0), port(port), ip(ip), backlog(backlog) {
#ifdef WIN32
        initWSA();
#endif
    }

    static const string rootDir;  // 资源所在根目录


    void showUsage();

    void addConnectionNumber();

    void subConnectionNumber();

    SOCKET getAcceptSocket() const;

    virtual void startServer() {
        info(" --------- starting Server ---------\n")
        try {
            info(" web_root dir is %s\n", getAbsPath(HttpServer::rootDir).c_str())
            acceptSocket = createListenSocket(port, ip, backlog);
        } catch (exception &e) {
            err(" --------- startServer failed, Err: %s ---------\n", e.what());
            safeExit(-1);
        }
        info(" --------- Server started ---------\n");
    }

    unsigned int getConnectionNumber() {
        lock_guard<mutex> lockGuard(numLock);
        return connectionNumber;
    }

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


void HttpServer::showUsage() {

}






