// 本文件包含了处理连接的子线程函数、MiniWebServer_v3 类
#pragma once

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <asm-generic/ioctls.h>
#include <sys/epoll.h>
#include <string>
#include <fcntl.h>
#include "../common/ThreadPool.cpp"
#include "../common/util.cpp"
#include "../common/WebServer.cpp"


#define MAX_EVENTS 10
typedef int SOCKET;
using namespace std;


/**
 * 对服务端封装成类
 */
class MiniWebServer_v3 : public WebServer{
private:
    ThreadPool threadPool;  // 线程池对象



public:
    explicit MiniWebServer_v3(int poolSize = 30) : threadPool(poolSize) {
    }



    void startServer(int port = 80, string ip = "", int maxSocketNumber = 65535) override;
};







/**
 * 开启 Web Server
 *
 * @param ip 本机 ip 地址
 * @param port 监听端口
 * @param maxSocketNumber 最大监听 socket 数量
*/
void MiniWebServer_v3::startServer(int port, string ip, int maxSocketNumber) {
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
                    //
                    // 获取连接 socket
                    //
                    sockaddr clientAddr;
                    socklen_t addrLen = sizeof(sockaddr);
                    int connSocket = accept(acceptSocket, &clientAddr, &addrLen);
                    if (connSocket == -1) {
                        if (errno != EAGAIN && errno != ECONNABORTED && errno != EPROTO && errno != EINTR) {
                            err("accept failed, Err:%s\n", getErrorInfo().c_str());
                        }
                        break;
                    }

                    //
                    // 将新 socket 加入到监听列表中
                    //
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
                //
                // 将 socket 放入任务队列中
                //
                bool rt = threadPool.submitTask(events[i].data.fd);
                if (!rt) {
                    err("submit failed, TaskQueue is full, close socket.\n");
                    SOCKET connSocket = events[i].data.fd;
                    HttpResponse::closeSocket(connSocket);
                }
            }
        }
    }
}




