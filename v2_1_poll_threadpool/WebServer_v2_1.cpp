#pragma once
#ifdef WIN32
#include <winsock2.h>
#else

#endif
#include <string>
#include <unordered_map>
#include "../include/http/HttpServer.h"
#include "../include/ThreadPool.h"

using namespace std;


class WebServer_v2_1 : public HttpServer {
private:
    ThreadPool<pair<SOCKET, long>> threadPool;  // 线程池对象
    unordered_map<SOCKET, long> acceptedTime;   // 记录每一个 socket 的 accept 时间

public:
    explicit WebServer_v2_1(int poolSize = 0) : threadPool(poolSize) {
#ifdef WIN32
        initWSA();
#endif
        showUsage();

        //
        // 设置任务完成时回调函数
        //
        function<void()> callback = bind(&HttpServer::subConnectionNumber, this);
        threadPool.setOnTaskFinishedCallback(callback);
    }

    void startServer(int port, string ip, int backlog) override;

    void showUsage() override;
};


/**
 * 开启 Web Server
 *
 * @param port 监听端口
 * @param ip 本机 ip 地址
 * @param backlog 最大监听 socket 数量
*/
void WebServer_v2_1::startServer(int port, string ip, int backlog) {
    info(" starting Server...\n")

    // TODO Poll

    //
    // 创建监听 socket，并设置为非阻塞
    //
    SOCKET acceptSocket = createListenSocket(port, backlog, ip);
    unsigned long ul = 1;
    ioctlsocket(acceptSocket, FIONBIO, &ul);    //设置成非阻塞模式
    info(" listenSocket: %d\n", acceptSocket)

    //
    // 创建相应集合
    //
    fd_set readable_fds;
    fd_set to_be_checked_fds;
    fd_set new_to_be_checked_fds;
    fd_set exceptional_fds;
    FD_ZERO(&readable_fds);
    FD_ZERO(&to_be_checked_fds);
    FD_SET(acceptSocket, &to_be_checked_fds);

    //
    // 循环 select
    //
    while (1) {
        FD_ZERO(&new_to_be_checked_fds);
        readable_fds = to_be_checked_fds;
        exceptional_fds = to_be_checked_fds;
        struct timeval selectTimeOut = {};
        selectTimeOut.tv_sec = 1;
        int rt = select(0, &readable_fds, NULL, &exceptional_fds, &selectTimeOut);
        if (rt < 0) {
            err("select failed, Err:%s\n", getErrorInfo().c_str());
            if (getErrorCode() == WSAENOTSOCK) {
                safeExit(-1);
            }
        } else if (rt == 0) {
            //
            // select 超时返回，关闭所有连接 socket
            //
            info("select time out\n");
            for (int i = 0; i < to_be_checked_fds.fd_count; i++) {
                if (to_be_checked_fds.fd_array[i] != acceptSocket) {
                    info(" [socket %s] wait time out\n", getSocketIPPort(to_be_checked_fds.fd_array[i]).c_str());
                    HttpResponse::closeSocket(to_be_checked_fds.fd_array[i]);
                }
            }
        } else {
            info(" select succeed\n");
            //
            // 只有 listenSocket 有连接事件，其余 socket 都没数据
            //
            if (readable_fds.fd_count == 1 && FD_ISSET(acceptSocket, &readable_fds) &&
                to_be_checked_fds.fd_count > 1) {
                // 去除 listenSocket，下次 select 就没有 listenSocket 了
                info(" listenSocket has events, but other sockets don't\n");
                FD_CLR(acceptSocket, &to_be_checked_fds);
                continue;
            }

            //
            // 遍历每一个 socket
            //
            int fdSeats = FD_SETSIZE - to_be_checked_fds.fd_count;
            for (int i = 0; i < to_be_checked_fds.fd_count; i++) {
                SOCKET socket = to_be_checked_fds.fd_array[i];
                if (FD_ISSET(socket, &exceptional_fds)) {
                    //
                    // 若该 socket 出现异常
                    //
                    info("[socket %s] exceptional socket: %d\n", getSocketIPPort(socket).c_str(),
                         socket);
                    HttpResponse::closeSocket(socket);
                } else if (FD_ISSET(socket, &readable_fds)) {
                    //
                    // 若该 socket 有可读事件
                    //
                    info("[socket %s] readable socket: %d\n", getSocketIPPort(socket).c_str(), socket);
                    if (socket == acceptSocket) {
                        //  若是监听 socket，则接收连接
                        info(" fdSeats: %d\n", fdSeats);
                        for (int j = 0; j < fdSeats; j++) {
                            //
                            // 接收新连接
                            //
                            sockaddr connAddr = {};
                            int addrLen = sizeof(connAddr);
                            SOCKET newConnSocket = accept(acceptSocket, &connAddr, &addrLen);
                            if (newConnSocket == INVALID_SOCKET) {
                                // WSAEWOULDBLOCK 表示 accept 队列为空
                                if (getErrorCode() != WSAEWOULDBLOCK) {
                                    err("[socket %s] accept failed, socket %d, Err:%s\n",
                                        getSocketIPPort(newConnSocket).c_str(),
                                        acceptSocket,
                                        getErrorInfo().c_str());
                                    safeExit(-1);
                                }
                                break;
                            }


                            //
                            // 将新 socket 放入下次 select 集合
                            //
                            acceptedTime[newConnSocket] = getCurrentTime();
                            addConnectionNumber();
                            info("[socket %s] new socket %d\n", getSocketIPPort(newConnSocket).c_str(), newConnSocket);
                            FD_SET(newConnSocket, &new_to_be_checked_fds);
                        }
                    } else {
                        //
                        // 如果是连接 socket，则表明有可读事件，提交任务
                        //
                        if (!threadPool.submitTask(make_pair(socket, acceptedTime[socket]))) {
                            info("[socket %s] submitTask failed, TaskQueue is full, close socket.\n",
                                 getSocketIPPort(socket).c_str());
                            closesocket(socket);
                        }
                    }
                } else {
                    //
                    // 若该连接 socket 没有任何事件，则放入下一次 select 集合中
                    // 当 listenSocket 也没有事件时，也会进入这块，要在这里判断一下，避免两次将 listenSocket 加入集合
                    //
                    if (socket != acceptSocket) {
                        FD_SET(socket, &new_to_be_checked_fds);
                        info("[socket %s] socket %d no data, go to next select\n", getSocketIPPort(socket).c_str(), socket);
                    }
                }
            }
        }


        //
        // 设置下轮监听集合
        //
        to_be_checked_fds = new_to_be_checked_fds;
        FD_SET(acceptSocket, &to_be_checked_fds);
    }
}

void WebServer_v2_1::showUsage() {

}

