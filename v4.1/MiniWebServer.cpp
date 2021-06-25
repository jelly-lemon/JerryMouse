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





DWORD WINAPI t_worker(LPVOID WorkThreadContext) {
    IO_DATA *data = NULL;
    void *lpCompletionKey = NULL;
    LPOVERLAPPED lpOverlapped = NULL;
    DWORD dwIoSize = 0;

    while (1) {
        // 从完成端口获取一个 IO 包，没有则会被挂起
        bool success = GetQueuedCompletionStatus(g_hIOCP, &dwIoSize, (PULONG_PTR) &lpCompletionKey,
                                  (LPOVERLAPPED *) &lpOverlapped, INFINITE);
        if (!success) {
            err("GetQueuedCompletionStatus failed, %s\n", getWSAErrorInfo().c_str())
            HttpResponse::closeSocket(data->client);
            delete data;
            continue;
        }

        // 如果客户端已经关闭，跳出本次循环
        // 【疑问】为啥 lpOverlapped 可以转成 IO_DATA ?难道是因为Overlapped是结构体第一个成员？对的
        data = (IO_DATA *) lpOverlapped;
        if (dwIoSize == 0) {
            info("[socket %s] client disconnected.\n", getClientIPPort(data->client).c_str());
            HttpResponse::closeSocket(data->client);
            delete data;
            continue;
        }

        // WSARecv 完成，也就是读操作完成
        if (data->opCode == RECV_FINISHED) {
            // 读取到的数据都保存在 lpIOContext 所指向内存中
            ZeroMemory(&data->Overlapped, sizeof(data->Overlapped));
            data->opCode = SEND_FINISHED;

            // 响应客户端
            IOCPHttpResponse response(data);
            string rawData(data->wsabuf.buf);
            try {
                response.handleRequest(rawData);
            } catch (exception &e) {
                err("handleRequest failed, %s, %s\n", e.what(), getWSAErrorInfo().c_str());
                HttpResponse::closeSocket(data->client);
                delete data;
                continue;
            }
        } else if (data->opCode == SEND_FINISHED) {
            // 回复成功
            info("[socket %s] reply finished\n", getClientIPPort(data->client).c_str());

            // 关闭连接
            HttpResponse::closeSocket(data->client);
            delete data;
            continue;
        }
    }
}


/**
 * 对服务端封装成类
 */
class MiniWebServer {
private:

    static string showAcceptSocketInfo(SOCKET acceptSocket);


public:
    explicit MiniWebServer() {
        initWSA();
    }

    void startServer(int port, int maxSocketNumber, string ip = "");

    static SOCKET createListenSocket(int port, int maxSocketNumber, string ip);
};


/**
 * 打印 acceptSocket 监听的 IP 和端口
 */
string MiniWebServer::showAcceptSocketInfo(SOCKET acceptSocket) {
    struct sockaddr_in socketAddr;
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
 * @param maxSocketNumber
 * @param ip
 * @return
 */
SOCKET MiniWebServer::createListenSocket(int port, int maxSocketNumber, string ip) {
    // 创建监听 socket
    SOCKADDR_IN addrSrv;
    if (ip.empty() || ip == "0.0.0.0")
        addrSrv.sin_addr.S_un.S_addr = htonl(INADDR_ANY);   // INADDR_ANY 表示监听所有网卡，也就是本机所有 IP 地址
    else if (ip == "localhost" || ip == "127.0.0.1") {
        addrSrv.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
    } else {
        addrSrv.sin_addr.S_un.S_addr = inet_addr(ip.c_str());
    }
    addrSrv.sin_family = AF_INET;
    addrSrv.sin_port = htons(port);
    SOCKET acceptSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);

    // 监听指定端口
    int n = bind(acceptSocket, (SOCKADDR *) &addrSrv, sizeof(SOCKADDR));
    if (n == SOCKET_ERROR) {
        err("bind port %d failed, %s\n", port, getWSAErrorInfo().c_str())

        WSACleanup();
        exit(-1);
    }

    // 开始监听
    // 半连接（SYN_RCVD状态）队列大小和全连接（ESTABLISHED状态）队列大小
    // backlog 指全连接队列大小
    listen(acceptSocket, maxSocketNumber); // 开始监听请求
    info("max accept socket number is %d\n", maxSocketNumber)
    showAcceptSocketInfo(acceptSocket);
    if (port == 80) {
        info("now, you can visit http://localhost to browse homepage.\n")
    } else {
        info("now, you can visit http://localhost:%d to browse homepage.\n", port)
    }
    info("web_root dir is %s\n", IOCPHttpResponse::rootDir.c_str())
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
    // 创建监听 socket
    SOCKET acceptSocket = createListenSocket(port, maxSocketNumber, ip);

    // 获取 CPU 逻辑核心数
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    int g_ThreadCount = sysInfo.dwNumberOfProcessors * 2;

    // 创建完成端口
    g_hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, g_ThreadCount);

    // 创建工作线程
    for (int i = 0; i < g_ThreadCount; ++i) {
        HANDLE hThread;
        DWORD dwThreadId;
        hThread = CreateThread(NULL, 0, WorkerThread, 0, 0, &dwThreadId);
        // 【注意】线程句柄（Handle）是内核对象，是系统资源，如果后面用不到了，就可以释放掉。不是结束线程的意思。
        // 线程句柄可以用改改变线程优先级等。
        CloseHandle(hThread);
    }

    // 等待客户端连接
    while (1) {
        SOCKET client = accept(acceptSocket, NULL, NULL);
        info("[socket %s] socket id %d, client connected\n", getClientIPPort(client).c_str(), client)

        // 将连接 socket 与完成端口绑定
        if (CreateIoCompletionPort((HANDLE) client, g_hIOCP, 0, 0) == NULL) {
            err("[socket %s] CreateIoCompletionPort failed, %s\n", getWSAErrorInfo().c_str())
            HttpResponse::closeSocket(client);
        } else {
            // 初始化 IO_DATA 结构体
            IO_DATA *data = new IO_DATA;
            memset(&data->Overlapped, 0, sizeof(data->Overlapped));
            data->opCode = RECV_FINISHED;
            int bufLen = 1024;
            data->wsabuf.buf = new char[bufLen];
            memset(data->wsabuf.buf, '\0', bufLen);
            data->wsabuf.len = bufLen;
            data->client = client;
            DWORD dwFlags = 0;

            // WSARecv 非阻塞，在这个 Socket 上提交一个读取数据的请求，然后内核就会去读取数据
            int nRet = WSARecv(client, &data->wsabuf, 1, NULL,
                               &dwFlags,
                               &data->Overlapped, NULL);
            if (nRet == SOCKET_ERROR && (ERROR_IO_PENDING != WSAGetLastError())) {
                err("WASRecv Failed, %s\n", getWSAErrorInfo().c_str())
                closesocket(client);
                delete data;
            }
        }
    }
}





