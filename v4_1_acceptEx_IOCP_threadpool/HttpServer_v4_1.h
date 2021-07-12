#pragma once

#include <winsock2.h>
#include <string>
#include <mswsock.h>
#include <wownt32.h>
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

    SOCKET createSocket() {
        //
        // 提前创建好 socket
        //
        SOCKET client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (client == INVALID_SOCKET) {
            err(" create socket failed, Err: %s\n", getErrorInfo().c_str());
            safeExit(-1);
        }

        //
        // 提交 AcceptEx
        //
        DWORD dwBytes;
        char lpOutputBuf[1024];
        int outBufLen = 1024;
        WSAOVERLAPPED olOverlap;
        memset(&olOverlap, 0, sizeof (olOverlap));
        bool bRetVal = lpfnAcceptEx(listenSocket, client, lpOutputBuf,
                                    outBufLen - ((sizeof(sockaddr_in) + 16) * 2),
                                    sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16,
                                    &dwBytes, &olOverlap);
        if (bRetVal == FALSE && getErrorCode() != WSA_IO_PENDING) {
            err(" AcceptEx failed, Err: %s\n", getErrorInfo().c_str());
            safeExit(-1);
        }


        if (CreateIoCompletionPort((HANDLE) client, g_hIOCP, 0, 0) == NULL) {
                err("bind clientSocket %d to IOCP failed, Err: %s\n",
                    client,
                    getErrorInfo().c_str())
                safeExit(-1);
            }

        return client;
    }

    void handleAccept() override {
        //
        // 创建完成端口和工作线程
        //
        int nWorker = getCPULogicCoresNumber() * 2 + 1;
        g_hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, nWorker);
        if (g_hIOCP == NULL) {
            err(" CreateIoCompletionPort failed, Err: %s\n", getErrorInfo().c_str());
            safeExit(-1);
        }
        for (int i = 0; i < 1; ++i) {
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
        }

        while (true) {
            SOCKET client = createSocket();

            while (true) {
                WSAEVENT EventArray[1];
                EventArray[0] = WSACreateEvent();
                int Index = WSAWaitForMultipleEvents(1, EventArray, FALSE, WSA_INFINITE, TRUE);

                if (Index == WSA_WAIT_FAILED) {
                    err(" WSAWaitForMultipleEvents failed, Err: %s\n", getErrorInfo().c_str());
                    safeExit(-1);
                }

                if (Index != WAIT_IO_COMPLETION) {
                    // The AcceptEx() event is ready, break the wait loop
                    info(" new socket\n");
                    break;
                }
                info(" waiting...\n");
            }


            //
            // 初始化 IO_DATA 结构体
            //
            IO_DATA *pIoData = new IO_DATA;
            memset(&pIoData->Overlapped, 0, sizeof(pIoData->Overlapped));
            pIoData->opCode = RECV_FINISHED;
            int bufLen = 8 * 1024;    // TODO 要是数据量比这个空间大怎么办？
            pIoData->wsabuf.buf = new char[bufLen];
            memset(pIoData->wsabuf.buf, '\0', bufLen);
            pIoData->wsabuf.len = bufLen;
            pIoData->client = client;

            //
            // 将连接 socket 与完成端口绑定
            //
//            if (CreateIoCompletionPort((HANDLE) pIoData->client, g_hIOCP, 0, 0) == NULL) {
//                err("[socket %s] bind clientSocket to IOCP failed, Err: %s\n",
//                    getSocketIPPort(pIoData->client).c_str(),
//                    getErrorInfo().c_str())
//                safeExit(-1);
//            }

            //
            // 向内核提交 recv 请求
            //
            // --------------------------------------------
            //
            // WSARecv 非阻塞，在这个 Socket 上提交一个读取数据的请求，然后内核就会去读取数据
            // 然后在子线程中用 GetQueuedCompletionStatus 阻塞等待读取结果
            //
            // --------------------------------------------
            DWORD dwFlags = 0;
            int nRet = WSARecv(pIoData->client, &pIoData->wsabuf, 1, NULL,
                               &dwFlags,
                               &pIoData->Overlapped, NULL);
            if (nRet == SOCKET_ERROR && (ERROR_IO_PENDING != WSAGetLastError())) {
                err(" WASRecv failed, Err: %s\n", getErrorInfo().c_str())
                closeSocket(pIoData->client);
                delete pIoData;
            }
        }
    }


    /**
     * 工作线程主函数
     *
     * @param WorkThreadContext
     * @return
     */
    static DWORD WINAPI worker_main(HttpServer_v4_1 *pServer) {
        IO_DATA *pIoData = NULL;
        void *lpCompletionKey = NULL;
        LPOVERLAPPED lpOverlapped = NULL;
        DWORD dwIoSize = 0;

        info(" new worker\n");
//        HANDLE stopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
//        while (WAIT_OBJECT_0 != WaitForSingleObject(stopEvent, 0)) {
//            info(" WaitForSingleObject\n");
//
//            //
//            // 从完成端口获取一个 IO 包，没有则会被挂起
//            //
//            bool success = GetQueuedCompletionStatus(g_hIOCP, &dwIoSize, (PULONG_PTR) &lpCompletionKey,
//                                                     (LPOVERLAPPED *) &lpOverlapped, INFINITE);
//            if (!success) {
//                err(" GetQueuedCompletionStatus failed, Err: %s\n", getErrorInfo().c_str())
//                continue;
//            }
//        }




        while (1) {
            //
            // 从完成端口获取一个 IO 包，没有则会被挂起
            //
            bool success = GetQueuedCompletionStatus(g_hIOCP, &dwIoSize, (PULONG_PTR) &lpCompletionKey,
                                                     (LPOVERLAPPED *) &lpOverlapped, INFINITE);
            if (!success) {
                err(" GetQueuedCompletionStatus failed, Err: %s\n", getErrorInfo().c_str())
                continue;
            } else {
                info(" GetQueuedCompletionStatus succeed\n");
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
                string rawData(pIoData->wsabuf.buf);
                try {
                    HttpRequest request(rawData, pIoData->client);
                    IOCPHttpResponse response(request, pIoData);
                    response.handleRequest();
                } catch (exception &e) {
                    err(" handleRequest failed, %s, %s\n", e.what(), getErrorInfo().c_str());
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
                pServer->subConnectionNumber();
                delete pIoData;

                pServer->createSocket();

                continue;
            }
        }
    }
};


HANDLE HttpServer_v4_1::g_hIOCP = NULL;
LPFN_ACCEPTEX HttpServer_v4_1::lpfnAcceptEx = NULL;
unordered_map<SOCKET, long> HttpServer_v4_1::acceptedTime;