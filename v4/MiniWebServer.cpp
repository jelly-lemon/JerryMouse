// 本文件包含了处理连接的子线程函数、MiniWebServer 类
#pragma once

#ifndef WIN32
#error please use windows
#endif
#include <winsock2.h>
#include <string>
#include "IOCPHttpResponse.cpp"

using namespace std;


HANDLE g_hIOCP;




/**
 * 工作线程主函数
 *
 * @param WorkThreadContext
 * @return
 */
DWORD WINAPI t_worker(LPVOID WorkThreadContext) {
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
            HttpResponse::closeSocket(pIoData->client);
            delete pIoData;
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
            info("[socket %s] client disconnected.\n", getClientIPPort(pIoData->client).c_str());
            HttpResponse::closeSocket(pIoData->client);
            Logger::subConnectionNumber();
            delete pIoData;
            continue;
        }

        //
        // WSARecv 完成，也就是读操作完成
        //
        if (pIoData->opCode == RECV_FINISHED) {
            // ----------------------------------------
            //
            // 读取到的数据都保存在 pIoData 所指向内存中
            //
            // ----------------------------------------
            ZeroMemory(&pIoData->Overlapped, sizeof(pIoData->Overlapped));
            pIoData->opCode = SEND_FINISHED;

            //
            // 响应客户端
            //
            IOCPHttpResponse response(pIoData);
            string rawData(pIoData->wsabuf.buf);
            try {
                response.handleRequest(rawData);
            } catch (exception &e) {
                err("handleRequest failed, %s, %s\n", e.what(), getErrorInfo().c_str());
                HttpResponse::closeSocket(pIoData->client);
                delete pIoData;
                continue;
            }
        } else if (pIoData->opCode == SEND_FINISHED) {
            //
            // 回复成功
            //
            info("[socket %s] reply finished\n", getClientIPPort(pIoData->client).c_str());

            //
            // 关闭连接，服务端响应一个请求后就立即关闭 socket
            //
            HttpResponse::closeSocket(pIoData->client);
            delete pIoData;
            continue;
        }
    }
}


/**
 * 对服务端封装成类
 */
class MiniWebServer {
private:

    static string showAcceptSocketIPPort(SOCKET acceptSocket);


public:
    explicit MiniWebServer() {
        initWSA();
    }

    void startServer(int port, int maxSocketNumber, string ip = "");

    static SOCKET createListenSocket(int port, int backlog, string ip);
};


/**
 * 打印 acceptSocket 监听的 IP 和端口
 */
string MiniWebServer::showAcceptSocketIPPort(SOCKET acceptSocket) {
    sockaddr_in socketAddr = {};
    int len = sizeof(socketAddr);
    getsockname(acceptSocket, (struct sockaddr *) &socketAddr, &len);
    string listenIpPort = string(inet_ntoa(socketAddr.sin_addr)) + ":" + to_string(ntohs(socketAddr.sin_port));
    info("server listen at %s\n", listenIpPort.c_str())

    return listenIpPort;
}

/**
 * 创建监听 socket
 *
 * @param port
 * @param backlog
 * @param ip
 * @return
 */
SOCKET MiniWebServer::createListenSocket(int port, int backlog, string ip) {
    //
    // 创建监听 socket
    //
    // ---------------------------------------------------
    //
    // INADDR_ANY 表示监听所有网卡，也就是本机所有 IP 地址
    //
    // ---------------------------------------------------
    SOCKADDR_IN addrSrv;
    if (ip.empty() || ip == "0.0.0.0")
        addrSrv.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
    else if (ip == "localhost" || ip == "127.0.0.1") {
        addrSrv.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
    } else {
        addrSrv.sin_addr.S_un.S_addr = inet_addr(ip.c_str());
    }
    addrSrv.sin_family = AF_INET;
    addrSrv.sin_port = htons(port);
    SOCKET acceptSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);

    //
    // 绑定监听端口
    //
    int n = bind(acceptSocket, (SOCKADDR *) &addrSrv, sizeof(SOCKADDR));
    if (n == SOCKET_ERROR) {
        err("bind port %d failed, Err:%s\n", port, getErrorInfo().c_str())
        safeExit(-1);
    }

    //
    // 开始监听请求
    //
    //-------------------------------------------------------------
    //
    // 有两个队列：半连接（SYN_RCVD 状态）队列和全连接（ESTABLISHED 状态）队列
    // backlog 指全连接队列大小
    //
    //-------------------------------------------------------------
    listen(acceptSocket, backlog);
    info("backlog is %d\n", backlog)
    showAcceptSocketIPPort(acceptSocket);
    if (port == 80) {
        info("now, you can visit http://localhost to browse homepage.\n")
    } else {
        info("now, you can visit http://localhost:%d to browse homepage.\n", port)
    }
    info("web_root dir is %s\n", HttpResponse::rootDir.c_str())
    info("waiting for connection...\n");

    return acceptSocket;
}


/**
 * 开启 Web Server
 *
 * @param ip 本机 ip 地址
 * @param port 监听端口
 * @param maxSocketNumber 最大监听 socket 数量
*/
void MiniWebServer::startServer(int port, int maxSocketNumber, string ip) {
    //
    // 创建监听 socket
    //
    SOCKET acceptSocket = createListenSocket(port, maxSocketNumber, ip);

    //
    // 创建完成端口和工作线程
    //
    int nWorker = getLogicCoresNumber() * 2;
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
        info("[socket %s] socket id %d, client connected\n", getClientIPPort(client).c_str(), client)
        Logger::addConnectionNumber();

        //
        // 将连接 socket 与完成端口绑定
        //
        if (CreateIoCompletionPort((HANDLE) client, g_hIOCP, 0, 0) == NULL) {
            err("[socket %s] CreateIoCompletionPort failed, %s\n", getErrorInfo().c_str())
            HttpResponse::closeSocket(client);
        } else {
            //
            // 初始化 IO_DATA 结构体
            //
            IO_DATA *pIoData = new IO_DATA;
            memset(&pIoData->Overlapped, 0, sizeof(pIoData->Overlapped));
            pIoData->opCode = RECV_FINISHED;
            int bufLen = 1024;
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
                HttpResponse::closeSocket(client);
                delete pIoData;
            }
        }
    }
}





