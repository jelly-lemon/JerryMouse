#pragma once

#include <winsock2.h>
#include <string>
#include "unordered_map"
#include "http/HttpServer.h"
#include "http/IOCPHttpResponse.h"


using namespace std;


class HttpServer_v4_1 : public HttpServer {
public:

    static HANDLE g_hIOCP;

    static unordered_map<SOCKET, long> acceptedTime;

    void startServer() {
        HttpServer::startServer();
//        setNonBlocking(acceptSocket);

        //
        // 创建完成端口和工作线程
        //
        int nWorker = getCPULogicCoresNumber() * 2;
        g_hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, nWorker);
        for (int i = 0; i < nWorker; ++i) {
            HANDLE hThread;
            DWORD dwThreadId;
            hThread = CreateThread(NULL, 0, t_worker, 0, 0, &dwThreadId);
            // -------------------------------------------------------
            //
            // 【注意】线程句柄（Handle）是内核对象，是系统资源。
            // 如果后面用不到了，就可以释放掉。不是结束线程的意思。
            // 线程句柄可以用于改变线程优先级等。
            //
            // -------------------------------------------------------
            CloseHandle(hThread);
        }

        //
        // 等待客户端连接
        //
        while (1) {
            SOCKET client = accept(acceptSocket, NULL, NULL);
            info("[socket %s] new socket %d\n", getSocketIPPort(client).c_str(), client)
            addConnectionNumber();

            //
            // 将连接 socket 与完成端口绑定
            //
            if (CreateIoCompletionPort((HANDLE) client, g_hIOCP, 0, 0) == NULL) {
                err("[socket %s] CreateIoCompletionPort failed, Err: %s\n", getSocketIPPort(client).c_str(),
                    getErrorInfo().c_str())
                subConnectionNumber();
                closeSocket(client);
            } else {
                //
                // 初始化 IO_DATA 结构体
                //
                IO_DATA *pIoData = new IO_DATA;
                memset(&pIoData->Overlapped, 0, sizeof(pIoData->Overlapped));
                pIoData->opCode = RECV_FINISHED;
                int bufLen = 8 * 1024;    // 要是数据量比这个空间大怎么办？
                pIoData->wsabuf.buf = new char[bufLen];
                memset(pIoData->wsabuf.buf, '\0', bufLen);
                pIoData->wsabuf.len = bufLen;
                pIoData->client = client;
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


    /**
     * 工作线程主函数
     *
     * @param WorkThreadContext
     * @return
     */
    static DWORD WINAPI t_worker(LPVOID WorkThreadContext) {
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
                info("[socket %s] socket %d disconnected.\n", getSocketIPPort(pIoData->client).c_str(),
                     pIoData->client);
                closeSocket(pIoData->client);
                subConnectionNumber();
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
                IOCPHttpResponse response(pIoData);
                string rawData(pIoData->wsabuf.buf);
                try {
                    response.handleRequest(rawData);
                } catch (exception &e) {
                    err("handleRequest failed, %s, %s\n", e.what(), getErrorInfo().c_str());
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
};


HANDLE HttpServer_v4_1::g_hIOCP;
unordered_map<SOCKET, long> HttpServer_v4_1::acceptedTime;