#pragma once

#include <thread>
#include <functional>
#include "../include/http/HttpResponse.cpp"
#include "../include/http/HttpServer.cpp"

using namespace std;

class WebServer_v1 : public WebServer {
private:
    mutex mutexWorkerNumber;
    int currentWorkerNumber;

public:
    WebServer_v1() : currentWorkerNumber(0) {
#ifdef WIN32
        initWSA();
#endif
    }
    void startServer(int port, string ip, int backlog) override;

    static void* worker_main(SOCKET connSocket, long acceptedTime, WebServer_v1 *pWebServer);

    void addWorkerNumber();

    void subWorkerNumber();
};


void WebServer_v1::startServer(int port, string ip, int backlog) {
    info(" starting Server...\n")

    //
    // 创建监听 socket
    //
    SOCKET acceptSocket = createListenSocket(port, backlog, ip);

    while (true) {
        sockaddr connAddr = {};
        int addrLen = sizeof(connAddr);
        //
        // 接收连接，启动处理线程
        //
        SOCKET newConnSocket = accept(acceptSocket, &connAddr, &addrLen);
        info("[socket %s] new socket %d\n", getSocketIPPort(newConnSocket).c_str(), newConnSocket);
        long acceptedTime = getTickCount();
        thread t(worker_main, newConnSocket, acceptedTime, this);
        t.detach();
    }
}

/**
 * 子线程函数
 */
void* WebServer_v1::worker_main(SOCKET connSocket, long acceptedTime, WebServer_v1 *pWebServer) {
    try {
        info(" [socket %s] socket %d wait time: %d ms\n", getSocketIPPort(connSocket).c_str(), connSocket, getTimeDiff(acceptedTime));
        pWebServer->addWorkerNumber();
        HttpResponse response(connSocket);
        response.handleRequest();
    } catch(exception &e) {
        info(" worker Err: %s\n", e.what());
    }

    //
    // 退出线程
    //
    info (" worker finished\n");
    pWebServer->subWorkerNumber();
    return NULL;
}

void WebServer_v1::addWorkerNumber() {
    lock_guard<mutex> lockGuard(mutexWorkerNumber);
    currentWorkerNumber++;
    info(" addWorkerNumber: %d\n", currentWorkerNumber);
}

void WebServer_v1::subWorkerNumber() {
    lock_guard<mutex> lockGuard(mutexWorkerNumber);
    currentWorkerNumber--;
    info(" subWorkerNumber: %d\n", currentWorkerNumber);
}