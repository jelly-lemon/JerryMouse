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


#define MAX_EVENTS 10
typedef int SOCKET;
using namespace std;


/**
 * 对服务端封装成类
 */
class MiniWebServer {
private:
    ThreadPool threadPool;  // 线程池对象

    static void showAcceptSocketIPPort(SOCKET acceptSocket);


public:
    explicit MiniWebServer(int poolSize = 30) : threadPool(poolSize) {
        initWSA();
    }

    void initWSA();

    void startServer(int port, int maxSocketNumber, string ip = "");

    static SOCKET createListenSocket(int port, int maxSocketNumber, string ip);
};


void MiniWebServer::showAcceptSocketIPPort(SOCKET acceptSocket) {
    string acceptIPPort = getAcceptIPPort(acceptSocket);
    if (!acceptIPPort.empty()) {
        info("server listen at %s\n", acceptIPPort.c_str())
    } else {
        err("can't get accept socket ip:port, Error: %s\n", acceptIPPort.c_str(), getErrorInfo().c_str())
    }
}

/**
 * 创建监听 socket
 *
 * @param port
 * @param maxSocketNumber
 * @param ip
 */
SOCKET MiniWebServer::createListenSocket(int port, int maxSocketNumber, string ip) {
    //
    // 创建监听 socket
    //
    sockaddr_in addrSrv = {};
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

    //
    // 绑定监听端口
    //
    int n = bind(acceptSocket, (sockaddr *) &addrSrv, sizeof(sockaddr));
    if (n == -1) {
        if (errno == EADDRINUSE) {
            err("port %d is in use, can't bind\n", port);
        } else {
            err("can't bind socket at %s:%d, Error:%s\n", ip.c_str(), port, getErrorInfo().c_str())
        }
        safeExit(-1);
    }

    //
    // 开始监听请求
    //
    // 半连接（SYN_RCVD状态）队列大小和全连接（ESTABLISHED状态）队列大小
    // backlog 指全连接队列大小
    listen(acceptSocket, maxSocketNumber);
    info("max accept socket number is %d\n", maxSocketNumber);
    showAcceptSocketIPPort(acceptSocket);
    if (port == 80) {
        info("now, you can visit http://127.0.0.1 to browse homepage.\n");
    } else {
        info("now, you can visit http://127.0.0.1:%d to browse homepage.\n", port);
    }
    info("web_root dir is %s\n", IOCPHttpResponse::rootDir.c_str());
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
    if (setNonBlocking(acceptSocket) == -1) {
        // 设置成非阻塞模式
        err("setNonBlocking failed, Err:%s\n", getErrorInfo().c_str());
        safeExit(-1);
    }

    //
    // 创建 epoll
    //
    int epfd = epoll_create(MAX_EVENTS);
    if (epfd == -1) {
        err("epoll_create failed, Err:%s\n", getErrorInfo().c_str());
        safeExit(-1);
    }

    //
    // 将 epoll 对象与 acceptSocket 绑定
    //
    epoll_event ev = {};
    ev.events = EPOLLIN;    //  事件类型
    ev.data.fd = acceptSocket;
    epoll_ctl(epfd, EPOLL_CTL_ADD, acceptSocket, &ev);

    //
    // 监听客户端连接
    //
    epoll_event events[MAX_EVENTS];
    while (1) {
        int nfds = epoll_wait(epfd, events, MAX_EVENTS, -1); // timeout 为 -1 表示无限等待
        if (nfds == -1) {
            err("epoll_wait failed, Err:%s\n", getErrorInfo().c_str());
            continue;
        }

        //
        // 遍历每一个事件
        //
        for (int i = 0; i < nfds; i++) {
            int fd = events[i].data.fd;
            // 如果是监听 socket
            if (fd == acceptSocket) {
                while (true) {
                    // 获取连接 socket
                    sockaddr clientAddr;
                    socklen_t addrLen = sizeof(sockaddr);
                    int connSocket = accept(acceptSocket, &clientAddr, &addrLen);
                    if (connSocket == -1) {
                        if (errno != EAGAIN && errno != ECONNABORTED && errno != EPROTO && errno != EINTR) {
                            err("accept failed, Err:%s\n", getErrorInfo().c_str());
                        }
                        break;
                    }

                    // 将新 socket 加入到监听列表中
                    if (setNonBlocking(connSocket) == -1) {
                        err("setNonBlocking failed, Err:%s\n", getErrorInfo().c_str());
                        continue;
                    }
                    ev.events = EPOLLIN | EPOLLET;
                    ev.data.fd = connSocket;
                    if (epoll_ctl(epfd, EPOLL_CTL_ADD, connSocket, &ev) == -1) {
                        err("epoll_ctl: add failed, Err:%s\n", getErrorInfo().c_str());
                        safeExit(-1);
                    }
                }
            } else if (events[i].events & EPOLLIN){
                // 将 socket 放入任务队列中
                bool rt = threadPool.submit(events[i].data.fd);
                if (!rt) {
                    err("submit failed, TaskQueue is full, close socket.\n");
                    closesocket(events[i].data.fd);
                }
            }
        }
    }
}




