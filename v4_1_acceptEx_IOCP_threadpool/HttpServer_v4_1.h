#pragma once

#include <winsock2.h>
#include <string>
#include <mswsock.h>
#include <wownt32.h>
#include <mstcpip.h>
#include "unordered_map"
#include "http/HttpServer.h"
#include "http/IOCPHttpResponse.h"


using namespace std;


class HttpServer_v4_1 : public HttpServer {
public:

    static HANDLE g_hIOCP;
    static LPFN_ACCEPTEX lpfnAcceptEx;
    static unordered_map<SOCKET, long> acceptedTime;

    explicit HttpServer_v4_1(int port = 80, string ip = "127.0.0.1", int backlog = 65535) : HttpServer(port, ip,
                                                                                                       backlog) {

    }


    /**
     * 提交 accept 请求
     *
     * @param client
     */
    void postAccept(SOCKET client) {
        //
        // 初始化 IO_DATA 结构体
        //
        IO_DATA *pIoData = new IO_DATA;
        memset(&pIoData->Overlapped, 0, sizeof(pIoData->Overlapped));
        pIoData->opCode = NEW_ACCEPT;
        int bufLen = 100;
        pIoData->wsabuf.buf = new char[bufLen];
        memset(pIoData->wsabuf.buf, '\0', bufLen);
        pIoData->wsabuf.len = bufLen;
        pIoData->client = client;
        pIoData->acceptCompletedTime = GetTickCount();

        //
        // 提交 AcceptEx
        //
        DWORD dwBytes;
        bool bRetVal = lpfnAcceptEx(listenSocket, pIoData->client, pIoData->wsabuf.buf,
                                    bufLen - ((sizeof(sockaddr_in) + 16) * 2),
                                    sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16,
                                    &dwBytes, &pIoData->Overlapped);
        if (bRetVal == FALSE && getErrorCode() != WSA_IO_PENDING) {
            err(" AcceptEx failed, Err: %s\n", getErrorInfo().c_str());
            safeExit(-1);
        } else {
            info(" AcceptEx succeed, socket: %d\n", pIoData->client);
        }
    }


    void run() override {
        //
        // 创建完成端口和工作线程
        //
        int nWorker = 2;
        g_hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
        if (g_hIOCP == NULL) {
            err(" CreateIoCompletionPort failed, Err: %s\n", getErrorInfo().c_str());
            safeExit(-1);
        }
        for (int i = 0; i < nWorker; ++i) {
            thread worker(worker_main, this);
            worker.detach();
        }

        //
        // 监听端口与 IOCP 绑定
        //
        HANDLE rt = CreateIoCompletionPort((HANDLE) listenSocket, g_hIOCP, (u_long) 0, 0);
        if (rt == NULL) {
            err(" bind IoCompletionPort to listenSocket failed, Err: %s\n", getErrorInfo().c_str());
            safeExit(-1);
        }

        //
        // 加载 AcceptEx 函数
        //
        GUID GuidAcceptEx = WSAID_ACCEPTEX;
        DWORD dwBytes;
        int iResult = WSAIoctl(listenSocket, SIO_GET_EXTENSION_FUNCTION_POINTER,
                               &GuidAcceptEx, sizeof(GuidAcceptEx),
                               &lpfnAcceptEx, sizeof(lpfnAcceptEx),
                               &dwBytes, NULL, NULL);
        if (iResult == SOCKET_ERROR) {
            err(" WSAIoctl failed, Err: %s\n", getErrorInfo().c_str());
            safeExit(-1);
        } else {
            info(" load AcceptEx succeed\n");
        }


        //
        // 提前创建 socket
        //
        for (int i = 0; i < getCPULogicCoresNumber() + 1; i++) {
            SOCKET client = createSocket();
            postAccept(client);

            //
            // 绑定端口
            //
            if (CreateIoCompletionPort((HANDLE) client, g_hIOCP, 0, 0) == NULL) {
                err(" bind socket %d to IOCP failed, Err: %s\n", client, getErrorInfo().c_str())
                closeSocket(client);
            } else {
                debug(" bind socket %d to IOCP succeed\n", client);
            }
        }



        //
        // 创建事件对象，让ServerShutdown程序能够关闭自己
        //
        HANDLE hEvent = CreateEvent(NULL, FALSE, FALSE, "ShutdownEvent");
        WaitForSingleObject(hEvent, INFINITE);
        CloseHandle(hEvent);

    }


    /**
     * 工作线程主函数
     *
     * @param WorkThreadContext
     * @return
     */
    static DWORD WINAPI worker_main(HttpServer_v4_1 *pServer) {
        info(" new worker\n");

        while (1) {
            IO_DATA *pIoData = NULL;
            void *lpCompletionKey = NULL;
            LPOVERLAPPED lpOverlapped = NULL;
            DWORD dwIoSize = 0;

            //
            // 从完成端口获取一个 IO 包，没有则会被挂起
            //
            bool success = GetQueuedCompletionStatus(g_hIOCP, &dwIoSize, (PULONG_PTR) &lpCompletionKey,
                                                     (LPOVERLAPPED *) &lpOverlapped, INFINITE);
            if (!success) {
                err(" GetQueuedCompletionStatus failed, Err: %s\n", getErrorInfo().c_str())
                continue;
            } else {
                info(" GetQueuedCompletionStatus succeed\n")
                pIoData = (IO_DATA *) lpOverlapped;
            }

            if (pIoData->opCode == NEW_ACCEPT) {
                //
                // TODO 要在这里判断是否读取了完整的数据
                //
                info(" new socket: %d\n", pIoData->client);

                info(" data: %s\n", pIoData->wsabuf.buf);

                //
                // 提交 Recv
                //
                DWORD dwFlags = 0; // 0: in 1: out
                DWORD dwBytes = 0;
                pIoData->opCode = RECV_FINISHED;
                int nRet = WSARecv(pIoData->client, &pIoData->wsabuf, 1, &dwBytes,
                                   &dwFlags,
                                   &pIoData->Overlapped, NULL);
                if (nRet == SOCKET_ERROR && (ERROR_IO_PENDING != WSAGetLastError())) {
                    err(" post WSARecv Failed, Err: %s\n", getErrorInfo().c_str())
                    closeSocket(pIoData->client);
                    delete pIoData;
                } else {
                    info(" post WSARecv succeed, socket: %d\n", pIoData->client);
                }
            } else if (pIoData->opCode == RECV_FINISHED) {
                info(" WSARecv finished, socket: %d\n", pIoData->client);
                //
                // WSARecv 完成，也就是读操作完成
                //
                //
                // 如果客户端已经关闭，跳出本次循环
                //
                // ---------------------------------------------
                //
                // 【注意】 Overlapped 必须是结构体第一个成员，
                // 否则 lpOverlapped 就无法转成 IO_DATA
                //
                // ---------------------------------------------
                if (dwIoSize == 0) {
                    info("[socket %s] socket %d disconnected.\n", getSocketIPPort(pIoData->client).c_str(),
                         pIoData->client);
                    closeSocket(pIoData->client);
                    delete pIoData;
                    continue;
                }


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
                string rawData(pIoData->wsabuf.buf);
                try {
                    HttpRequest request(rawData, pIoData->client);
                    IOCPHttpResponse response(request, pIoData);
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
                pServer->subCurrentConnectionNumber();
                delete pIoData;

                SOCKET client = createSocket();
                pServer->postAccept(client);

                continue;
            }
        }
    }
};


HANDLE HttpServer_v4_1::g_hIOCP = NULL;
LPFN_ACCEPTEX HttpServer_v4_1::lpfnAcceptEx = NULL;
unordered_map<SOCKET, long> HttpServer_v4_1::acceptedTime;