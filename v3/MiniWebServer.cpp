// 本文件包含了处理连接的子线程函数、MiniWebServer 类
#pragma once

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <asm-generic/ioctls.h>
#include <sys/epoll.h>
#include <string>
#include <fcntl.h>
#include "../ThreadPool.cpp"

#define SOCKET int
#define MAX_EVENTS 10

using namespace std;


/**
 * 对服务端封装成类
 */
class MiniWebServer {
private:
    ThreadPool threadPool;  // 线程池对象

    static void showAcceptSocketInfo(SOCKET acceptSocket);


public:
    explicit MiniWebServer(int poolSize = 30): threadPool(poolSize) {
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
    sockaddr_in socketAddr;
    socklen_t len = sizeof(socketAddr);
    getsockname(acceptSocket, (struct sockaddr *) &socketAddr, &len);
    char msg[100];
    sprintf(msg, "server listen at %s:%d\n", inet_ntoa(socketAddr.sin_addr), ntohs(socketAddr.sin_port));
    Log::record(msg);
}

SOCKET MiniWebServer::createListenSocket(int port, int maxSocketNumber, string ip) {
    // 创建监听 socket
    sockaddr_in addrSrv;
    if (ip.empty() || ip == "0.0.0.0")
        addrSrv.sin_addr.s_addr = htonl(INADDR_ANY);   // INADDR_ANY 表示监听所有网卡，也就是本机所有 IP 地址
    else if (ip == "localhost" || ip == "127.0.0.1") {
        addrSrv.sin_addr.s_addr = inet_addr("127.0.0.1");
    } else {
        addrSrv.sin_addr.s_addr = inet_addr(ip.c_str());
    }
    addrSrv.sin_family = AF_INET;
    addrSrv.sin_port = htons(port);
    SOCKET acceptSocket = socket(AF_INET, SOCK_STREAM, 0);

    // 监听指定端口
    int n;
    n = bind(acceptSocket, (sockaddr *) &addrSrv, sizeof(sockaddr));
    if (n == -1) {
        if (errno == EADDRINUSE) {
            Log::info("port %d is in use, can't bind\n", port);
        } else {
            Log::info("can't bind socket at %s:%d, error code:%d\n", ip.c_str(), port, errno);
        }
        // TODO 退出时要保证最后一条日志正常写入文件
        exit(0);
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

    unsigned long ul=1;
    // win 环境下用 ioctlsocket， Linux 环境下用 fcntl
    fcntl(acceptSocket, FIONBIO, &ul);   // 设置成非阻塞模式


    int epfd = epoll_create(MAX_EVENTS);
    if (epfd == -1) {
        Log::info("epoll_create failed, err:%d\n", errno);
        exit(EXIT_FAILURE);
    }
    struct epoll_event ev, events[MAX_EVENTS];
    ev.events = EPOLLIN;
    ev.data.fd = acceptSocket;
    epoll_ctl(epfd, EPOLL_CTL_ADD, acceptSocket, &ev);

    while (1) {
        int nfds = epoll_wait(epfd, events, MAX_EVENTS, -1);
        if (nfds == -1) {
            Log::info("epoll_wait failed, errno:%d\n", errno);
            exit(EXIT_FAILURE);
        }
        for (int i = 0; i < nfds; i++) {
            int fd = events[i].data.fd;
            // 如果是监听 socket
            if (fd == acceptSocket) {
                while (true) {
                    sockaddr clientAddr;
                    socklen_t addrLen = sizeof(sockaddr);
                    int connSocket = accept(acceptSocket, &clientAddr, &addrLen);

                }
            }
        }
    }
}




