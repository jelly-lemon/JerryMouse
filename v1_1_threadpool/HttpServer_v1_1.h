#pragma once

#include <string>
#include "../include/Logger.h"
#include "../include/http/HttpServer.h"
#include "../include/ThreadPool.h"


using namespace std;


/**
 * 对服务端封装成类
 */
class HttpServer_v1_1 : public HttpServer{
private:
    ThreadPool *pThreadPool;



    void handleAccept() override {
        if (pThreadPool == NULL) {
            err(" please setThreadPool\n");
            safeExit(-1);
        }

        //
        // 接收连接
        //
        while (true) {
            sockaddr connAddr = {};
            int addrLen = sizeof(connAddr);
            SOCKET newConnSocket = accept(listenSocket, &connAddr, &addrLen);
            info("[socket %s] new socket %d\n", getSocketIPPort(newConnSocket).c_str(), newConnSocket);
            long acceptedTime = getTickCount();
            //
            // 提交任务
            //
            pThreadPool->submitTask(bind(task, newConnSocket, acceptedTime));
        }
    }

    static void task(SOCKET clientSocket, long acceptedTime) {
        long waitTime = getTimeDiff(acceptedTime);
        info("[socket %s] socket %d wait time: %d ms\n", getSocketIPPort(clientSocket).c_str(), clientSocket, waitTime);
        try {
            HttpResponse httpResponse(clientSocket);
            httpResponse.handleRequest();
        } catch (exception &e) {
            err(" task() failed, Err: %s\n", e.what());
        }
    }



public:
    explicit HttpServer_v1_1(int port = 80, string ip = "127.0.0.1", int backlog = 65535) :
            HttpServer(port, ip, backlog) {

    }

    /**
     * 设置线程池对象
     *
     * @param pThreadPool
     */
    void setThreadPool(ThreadPool *pThreadPool) {
        this->pThreadPool = pThreadPool;
    }

};


