// 本文件包含了处理连接的子线程函数、HttpServer_v3 类
#pragma once

#include <sys/socket.h>
#include <sys/epoll.h>
#include <string>
#include <unordered_map>
#include "ThreadPool.h"
#include "util.h"
#include "http/HttpServer.h"
#include "SocketHelper.h"


#define MAX_EVENTS 10
typedef int SOCKET;
using namespace std;


/**
 * 对服务端封装成类
 */
class HttpServer_v3 : public HttpServer{
private:
    ThreadPool threadPool;  // 线程池对象

    unordered_map<SOCKET, long> acceptedTime;

    int epfd;

    EPOLL_EVENTS triggerMode;


public:
    explicit HttpServer_v3(int port = 80, string ip = "127.0.0.1", int backlog = 65535) :
    threadPool(0),
    HttpServer(port, ip, backlog) {
        triggerMode = EPOLLET;
    }

    /**
     * 设置触发模式
     *
     * @param triggerMode
     */
    void setTriggerMode(EPOLL_EVENTS triggerMode) {
        this->triggerMode = triggerMode;
        info(" set triggerMode: d%\n", triggerMode);
    }

private:

    /**
     * 移除监听事件列表
     */
    void removeEvent(SOCKET client, uint32_t events = EPOLLIN | EPOLLET) {
        epoll_event ev = {};
        ev.events = events;
        ev.data.fd = client;

        if (epoll_ctl(epfd, EPOLL_CTL_DEL, client, &ev) != -1) {
            debug(" removeEvent socket %d failed, Err: %s\n", client, getErrorInfo().c_str());
        } else {
            debug(" removeEvent socket %d succeed\n", client);
        }
    }

    void handleWrite(SOCKET client) {

    }

    /**
     * 处理可读事件
     */
    void handleRead(SOCKET client) {
        //
        // 将 socket 放入任务队列中
        //
        function<void()> taskFinishedCallback = bind(&HttpServer_v3::removeEvent, this, client, EPOLLIN | triggerMode);
        function<void()> newTask = bind(HttpResponse::HandleRequest, client, acceptedTime[client], taskFinishedCallback);
        bool rt = threadPool.submitTask(newTask);
        if (!rt) {
            err("submit failed, TaskQueue is full, close socket.\n");
            closeSocket(client);
        }
    }

    void handleAccept() {
        while (true) {
            //
            // 获取连接 socket
            //
            sockaddr clientAddr;
            socklen_t addrLen = sizeof(sockaddr);
            int client = accept(listenSocket, &clientAddr, &addrLen);
            if (client == -1) {
                //
                // EAGAIN:
                // ECONNABORTED:
                // EPROTO:
                // EINTR:
                //
                if (errno != EAGAIN && errno != ECONNABORTED && errno != EPROTO && errno != EINTR) {
                    err(" accept failed, Err: %s\n", getErrorInfo().c_str());
                }
                break;
            } else {
                info(" new socket: %d\n", client);
                acceptedTime[client] = getCurrentTime();
            }


            //
            // 将新 socket 加入到监听列表中
            //
            // EPOLLIN: 可读事件
            // EPOLLET: 边缘触发
            //
            epoll_event ev = {};
            ev.events = EPOLLIN | triggerMode;
            ev.data.fd = client;
            if (epoll_ctl(epfd, EPOLL_CTL_ADD, client, &ev) == -1) {
                err(" epoll_ctl add socket %d failed, Err:%s\n", client, getErrorInfo().c_str());
                safeExit(-1);
            } else {
                debug(" epoll_ctl add socket %d succeed\n", client);
            }
        }
    }

    void run() override {
        //
        // 设置监听 socket 为非阻塞
        //
        if (setNonBlocking(listenSocket) == -1) {
            safeExit(-1);
        }

        //
        // 创建 epoll
        //
        epfd = epoll_create(MAX_EVENTS);
        if (epfd == -1) {
            err(" epoll_create failed, Err:%s\n", getErrorInfo().c_str());
            safeExit(-1);
        }

        //
        // 将 epoll 对象与 listenSocket 绑定
        //
        epoll_event ev = {};
        ev.events = EPOLLIN;    //  事件类型
        ev.data.fd = listenSocket;
        epoll_ctl(epfd, EPOLL_CTL_ADD, listenSocket, &ev);

        //
        // 等待事件
        //
        epoll_event events[MAX_EVENTS];
        while (1) {
            int nfds = epoll_wait(epfd, events, MAX_EVENTS, -1); // timeout 为 -1 表示无限等待
            if (nfds == -1) {
                err(" epoll_wait failed, Err:%s\n", getErrorInfo().c_str());
                continue;
            } else {
                debug(" epoll_wait succeed, nfds: %d\n", nfds);
            }

            //
            // 遍历每一个事件
            //
            for (int i = 0; i < nfds; i++) {
                int fd = events[i].data.fd;
                //
                // 如果有新连接
                //
                if (fd == listenSocket) {
                    handleAccept();
                } else if (events[i].events & EPOLLIN){
                    //
                    // 如果是可读事件
                    //
                    handleRead(events[i].data.fd);
                } else if (events[i].events & EPOLLOUT) {
                    //
                    // 可写事件
                    //
                    handleWrite(events[i].data.fd);
                } else {
                    debug(" unknown socket %d event: %d", events[i].data.fd, events[i].events);
                }
            }
        }
    }
};













