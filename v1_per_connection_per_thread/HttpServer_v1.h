#pragma once

#include <thread>
#include <functional>
#include "../include/http/HttpResponse.h"
#include "../include/http/HttpServer.h"

using namespace std;

class HttpServer_v1 : public HttpServer {
private:
    mutex mutexWorkerNumber;
    int currentWorkerNumber;

public:
    HttpServer_v1(int port = 80, string ip = "127.0.0.1", int backlog = 65535) :
    currentWorkerNumber(0), HttpServer(port, ip, backlog) {

    }


    void startServer();

    static void* worker_main(SOCKET connSocket, long acceptedTime, HttpServer_v1 *pHttpServer);

    void addWorkerNumber();

    void subWorkerNumber();
};


void HttpServer_v1::startServer() {
    HttpServer::startServer();

    while (true) {
        sockaddr connAddr = {};
        int addrLen = sizeof(connAddr);
        //
        // 接收连接，启动处理线程
        //
        SOCKET newConnSocket = accept(listenSocket, &connAddr, &addrLen);
        info("[socket %s] new socket %d\n", getSocketIPPort(newConnSocket).c_str(), newConnSocket);
        long acceptedTime = getCurrentTime();
//        thread t(worker_main, newConnSocket, acceptedTime, this);
//        t.detach();
    }
}

/**
 * 子线程函数
 */
void* HttpServer_v1::worker_main(SOCKET connSocket, long acceptedTime, HttpServer_v1 *pHttpServer) {
    info(" [socket %s] socket %d wait time: %d ms\n", getSocketIPPort(connSocket).c_str(), connSocket, getTimeDiff(acceptedTime));
    pHttpServer->addWorkerNumber();


    //
    // 处理请求
    //
    try {
        HttpResponse response(connSocket);
        response.handleRequest();
        closeSocket(connSocket);
    } catch(exception &e) {
        info(" handleRequest Err: %s\n", e.what());
    }

    //
    // 退出线程
    //
    info (" worker finished\n");
    pHttpServer->subWorkerNumber();
    return NULL;
}

void HttpServer_v1::addWorkerNumber() {
    lock_guard<mutex> lockGuard(mutexWorkerNumber);
    currentWorkerNumber++;
    info(" addWorkerNumber: %d\n", currentWorkerNumber);
}

void HttpServer_v1::subWorkerNumber() {
    lock_guard<mutex> lockGuard(mutexWorkerNumber);
    currentWorkerNumber--;
    info(" subWorkerNumber: %d\n", currentWorkerNumber);
}