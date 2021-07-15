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



    void run() override {
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
            if (newConnSocket != SOCKET_ERROR) {
                info("[socket %s] new socket %d\n", getSocketIPPort(newConnSocket).c_str(), newConnSocket);
                addConnectionNumber();
                addTotalRequest();
                long acceptedTime = getCurrentTime();
                //
                // 提交任务
                //
                function<void()> onTaskFinishedCallback = bind(&HttpServer::subConnectionNumber, this);
                function<void()> task = bind(HttpResponse::HandleRequest, newConnSocket, acceptedTime, onTaskFinishedCallback);
                pThreadPool->submitTask(task);
            } else {
                err(" accept failed, Err: %s\n", getErrorInfo().c_str());
            }

        }
    }


public:
    explicit HttpServer_v1_1(int port = 80, string ip = "127.0.0.1", int backlog = 65535) :
            HttpServer(port, ip, backlog), pThreadPool(NULL) {

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


