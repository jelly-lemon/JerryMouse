// 本文件包含了处理连接的子线程函数、MiniWebServer 类
#pragma once

#ifndef WIN32
#error please use windows
#endif
#include <winsock2.h>
#include <string>

#include "../ThreadPool.cpp"

using namespace std;


HANDLE g_hIOCP;





DWORD WINAPI WorkerThread(LPVOID WorkThreadContext) {
    IO_DATA *lpIOContext = NULL;
    DWORD nBytes = 0;
    DWORD dwFlags = 0;
    int nRet = 0;
    char buffer[1024];

    DWORD dwIoSize = 0;
    void *lpCompletionKey = NULL;
    LPOVERLAPPED lpOverlapped = NULL;

    while (1) {
        // 从完成端口获取一个 IO 包，没有则会被挂起
        GetQueuedCompletionStatus(g_hIOCP, &dwIoSize, (PULONG_PTR) &lpCompletionKey,
                                  (LPOVERLAPPED *) &lpOverlapped, INFINITE);

        // 如果客户端已经关闭，跳出本次循环
        // 【疑问】为啥 lpOverlapped 可以转成 IO_DATA ?难道是因为Overlapped是结构体第一个成员？对的
        lpIOContext = (IO_DATA *) lpOverlapped;
        if (dwIoSize == 0) {
            Log::info("[socket %s] client disconnected.\n", getClientIPPort(lpIOContext->client).c_str());
            int n = closesocket(lpIOContext->client);
            if (n == SOCKET_ERROR) {
                Log::info("[socket %s] close socket err, %s\n", getClientIPPort(lpIOContext->client).c_str(),
                          getWSAErrorInfo().c_str());
            } else {
                Log::info("[socket %s] we closed socket.\n", getClientIPPort(lpIOContext->client).c_str());
            }

            delete lpIOContext;
            continue;
        }

        // WSARecv 完成，也就是读操作完成
        if (lpIOContext->opCode == RECV_FINISHED) {
            ZeroMemory(&lpIOContext->Overlapped, sizeof(lpIOContext->Overlapped));
            char s[100] = "hello, I'm server.";
            lpIOContext->wsabuf.buf = s;
            lpIOContext->wsabuf.len = strlen(s) + 1;
            lpIOContext->opCode = SEND_FINISHED;
            lpIOContext->nBytes = strlen(s) + 1;

            // 响应客户端
            IOCPHttpResponse response(lpIOContext);
            string rawData(lpIOContext->wsabuf.buf);
            try {
                response.handleRequest(rawData);
            } catch (exception &e) {
                Log::info("handleRequest failed, %s\n", getWSAErrorInfo().c_str());
                delete lpIOContext;
                continue;
            }
        } else if (lpIOContext->opCode == SEND_FINISHED) {
            Log::info("[socket %s]reply\n%s\n", getClientIPPort(lpIOContext->client).c_str(), lpIOContext->wsabuf.buf);

            // 关闭连接
            

        }
    }
    return 0;
}


/**
 * 对服务端封装成类
 */
class MiniWebServer {
private:

    static void showAcceptSocketInfo(SOCKET acceptSocket);


public:
    explicit MiniWebServer() {
        initWSA();
    }

    void initWSA();

    void startServer(int port, int maxSocketNumber, string ip = "");

    static SOCKET createListenSocket(int port, int maxSocketNumber, string ip);
};


/**
 * 打印 acceptSocket 监听的 IP 和端口
 */
void MiniWebServer::showAcceptSocketInfo(SOCKET acceptSocket) {
    struct sockaddr_in socketAddr;
    int len = sizeof(socketAddr);
    getsockname(acceptSocket, (struct sockaddr *) &socketAddr, &len);
    char msg[100];
    sprintf(msg, "server listen at %s:%d\n", inet_ntoa(socketAddr.sin_addr), ntohs(socketAddr.sin_port));
    Log::record(msg);
}

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
    int n;
    n = bind(acceptSocket, (SOCKADDR *) &addrSrv, sizeof(SOCKADDR));
    if (n == SOCKET_ERROR) {
        int error_code = WSAGetLastError();
        char msg[101];
        if (error_code == WSAEADDRINUSE) {
            snprintf(msg, 100, "port %d is in use, can't bind\n", port);
        } else {
            snprintf(msg, 100, "can't bind socket at %s:%d, WSA error code:%d\n", ip.c_str(), port, error_code);
        }
        Log::record(msg);
        WSACleanup();
        exit(-1);
    }

    // 半连接（SYN_RCVD状态）队列大小和全连接（ESTABLISHED状态）队列大小
    // backlog 指全连接队列大小
    listen(acceptSocket, maxSocketNumber); // 开始监听请求

    char msg[101] = {'\0'};
    snprintf(msg, 100, "max accept socket number is %d\n", maxSocketNumber);
    Log::record(msg);
    showAcceptSocketInfo(acceptSocket);
    if (port == 80) {
        snprintf(msg, 100, "now, you can visit http://localhost to browse homepage.\n");
    } else {
        snprintf(msg, 100, "now, you can visit http://localhost:%d to browse homepage.\n", port);
    }
    Log::record(msg);
    snprintf(msg, 100, "web_root dir is %s\n", IOCPHttpResponse::rootDir.c_str());
    Log::record(msg);
    Log::record("waiting for connection...\n");

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

    while (1) {
        // 等待客户端连接
        SOCKET client = accept(acceptSocket, NULL, NULL);
        cout << "Client connected." << endl;

        // 将连接 socket 与完成端口绑定
        if (CreateIoCompletionPort((HANDLE) client, g_hIOCP, 0, 0) == NULL) {
            cout << "Binding Client Socket to IO Completion Port Failed::Reason Code::" << GetLastError() << endl;
            closesocket(client);
        } else {
            // 初始化 IO_DATA 结构体
            IO_DATA *data = new IO_DATA;
            memset(buffer, '\0', 1024);
            memset(&data->Overlapped, 0, sizeof(data->Overlapped));
            data->opCode = RECV_FINISHED;
            data->nBytes = 0;
            data->wsabuf.buf = buffer;
            data->wsabuf.len = sizeof(buffer);
            data->client = client;
            DWORD nBytes = 1024, dwFlags = 0;

            // WSARecv 非阻塞，在这个 Socket 上提交一个读取数据的请求，然后内核就会去读取数据
            int nRet = WSARecv(client, &data->wsabuf, 1, &nBytes,
                               &dwFlags,
                               &data->Overlapped, NULL);
            if (nRet == SOCKET_ERROR && (ERROR_IO_PENDING != WSAGetLastError())) {
                cout << "WASRecv Failed::Reason Code::" << WSAGetLastError() << endl;
                closesocket(client);
                delete data;
            }
        }
    }

}


/**
 * 初始化 socket 调用环境
 */
void MiniWebServer::initWSA() {
    WORD wVersionRequested; // WORD 就是 unsigned short，无符号短整型
    WSADATA wsaData;    // 一个结构体。这个结构被用来存储被 WSAStartup 函数调用后返回的 Windows Sockets 数据。
    wVersionRequested = MAKEWORD(2, 2); // 将两个 byte 型合并成一个 word 型,一个在高8位(b),一个在低8位(a)。整数 1 是 byte 类型吗？

    // 初始化套接字环境
    int n = WSAStartup(wVersionRequested, &wsaData);  // 即WSA(Windows Sockets Asynchronous，Windows异步套接字)的启动命令
    if (n != 0) {
        int err = WSAGetLastError();
        char msg[101] = {'\0'};
        snprintf(msg, 100, "WSAStartup failed. err:%d\n", err);
        Log::record(msg);
        exit(-1);
    }

    // 检查是否初始化成功
    if (LOBYTE(wsaData.wVersion) != 1 || HIBYTE(wsaData.wVersion) != 1) {
        WSACleanup();   // 功能是终止 Winsock 2 DLL (Ws2_32.dll) 的使用
        int err = WSAGetLastError();
        char msg[101] = {'\0'};
        snprintf(msg, 100, "WSAStartup failed. err:%d\n", err);
        Log::record(msg);
        exit(-1);
    }
}


