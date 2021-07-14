// 本文件包含了处理连接的子线程函数、HttpServer_v4 类
#pragma once

#ifndef WIN32
#error please use windows
#endif
#include <winsock2.h>
#include <string>
#include "http/IOCPHttpResponse.h"

using namespace std;


HANDLE g_hIOCP;







/**
 * 对服务端封装成类
 */
class HttpServer_v4: public HttpServer{
private:



public:
    explicit HttpServer_v4(int port = 80, string ip = "127.0.0.1", int backlog = 65535): HttpServer(port, ip, backlog) {

    }

    /**
 * 工作线程主函数
 *
 * @param WorkThreadContext
 * @return
 */
    static DWORD WINAPI t_worker(HttpServer_v4* pServer) {
        IO_DATA *pIoData = NULL;
        void *lpCompletionKey = NULL;
        LPOVERLAPPED lpOverlapped = NULL;
        DWORD dwIoSize = 0;

        while (1) {
            //
            // 从完成端口获取一个 IO 包，没有则会被挂起
            //
            bool success = GetQueuedCompletionStatus(g_hIOCP, &dwIoSize, (PULONG_PTR) &lpCompletionKey,
                                                     (LPOVERLAPPED *) &lpOverlapped, INFINITE);
            if (!success) {
                err("GetQueuedCompletionStatus failed, %s\n", getErrorInfo().c_str())
                continue;
            }

            //
            // 如果客户端已经关闭，跳出本次循环
            //
            // ---------------------------------------------
            //
            // 【注意】 Overlapped 必须是结构体第一个成员，
            // 否则 lpOverlapped 就无法转成 IO_DATA
            //
            // ---------------------------------------------
            pIoData = (IO_DATA *) lpOverlapped;
            if (dwIoSize == 0) {
                info("[socket %s] client disconnected.\n", getSocketIPPort(pIoData->client).c_str());
                closeSocket(pIoData->client);
                pServer->subConnectionNumber();
                delete pIoData;
                continue;
            }

            //
            // WSARecv 完成，也就是读操作完成
            //
            if (pIoData->opCode == RECV_FINISHED) {
                pIoData->beginHandleTime = GetTickCount();
                DWORD waitingTime = pIoData->beginHandleTime - pIoData->acceptCompletedTime;
                info("[socket %s] waiting time: %d ms\n", getSocketIPPort(pIoData->client).c_str(), waitingTime);

                // ----------------------------------------
                //
                // 读取到的数据都保存在 pIoData 所指向内存中
                //
                // ----------------------------------------
                ZeroMemory(&pIoData->Overlapped, sizeof(pIoData->Overlapped));
                pIoData->opCode = SEND_FINISHED;
                cpuRun(1000);

                //
                // 响应客户端
                //
                try {
                    string rawData(pIoData->wsabuf.buf);
                    HttpRequest request(rawData, pIoData->client);
                    HttpResponse response(request);
                    response.handleRequest();
                } catch (exception &e) {
                    err(" handleRequest failed, Err: %s\n", e.what());
                    closeSocket(pIoData->client);
                    delete pIoData;
                    continue;
                }
            } else if (pIoData->opCode == SEND_FINISHED) {
                //
                // 回复成功
                //
                info("[socket %s] reply finished\n", getSocketIPPort(pIoData->client).c_str());
                DWORD handleTime = GetTickCount() - pIoData->beginHandleTime;
                info("[socket %s] handle time: %d ms\n", getSocketIPPort(pIoData->client).c_str(), handleTime);

                //
                // 关闭连接，服务端响应一个请求后就立即关闭 socket
                //
                closeSocket(pIoData->client);
                delete pIoData;
                continue;
            }
        }
    }



private:
    void run() override;
};


void HttpServer_v4::run() {
    //
    // 创建完成端口和工作线程
    //
    int nWorker = 2;
    g_hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, nWorker);
    for (int i = 0; i < nWorker; ++i) {
        thread t(t_worker, this);
        t.detach();
    }

    //
    // 等待客户端连接
    //
    while (1) {
        SOCKET client = accept(listenSocket, NULL, NULL);
        info("[socket %s] socket id %d, client connected\n", getSocketIPPort(client).c_str(), client)
        addConnectionNumber();

        //
        // 将连接 socket 与完成端口绑定
        //
        if (CreateIoCompletionPort((HANDLE) client, g_hIOCP, 0, 0) == NULL) {
            err("[socket %s] CreateIoCompletionPort failed, %s\n", getErrorInfo().c_str())
            closeSocket(client);
        } else {
            //
            // 初始化 IO_DATA 结构体
            //
            IO_DATA *pIoData = new IO_DATA;
            memset(&pIoData->Overlapped, 0, sizeof(pIoData->Overlapped));
            pIoData->opCode = RECV_FINISHED;
            int bufLen = 8 * 1024 * 100;
            pIoData->wsabuf.buf = new char[bufLen];
            memset(pIoData->wsabuf.buf, '\0', bufLen);
            pIoData->wsabuf.len = bufLen;
            pIoData->client = client;
            pIoData->acceptCompletedTime = GetTickCount();
            DWORD dwFlags = 0;

            //
            // 向内核提交 recv 请求
            //
            // --------------------------------------------
            //
            // WSARecv 非阻塞，在这个 Socket 上提交一个读取数据的请求，然后内核就会去读取数据
            // 然后在子线程中用 GetQueuedCompletionStatus 阻塞等待读取结果
            //
            // --------------------------------------------
            int nRet = WSARecv(client, &pIoData->wsabuf, 1, NULL,
                               &dwFlags,
                               &pIoData->Overlapped, NULL);
            if (nRet == SOCKET_ERROR && (ERROR_IO_PENDING != WSAGetLastError())) {
                err("WASRecv Failed, %s\n", getErrorInfo().c_str())
                closeSocket(client);
                delete pIoData;
            }
        }
    }
}





