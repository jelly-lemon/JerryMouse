#pragma once

#include <winsock2.h>
#include <string>
#include "../common/MiniWebServer.cpp"
#include "../common/ThreadPool.cpp"
#include "../common/util.cpp"

using namespace std;


class MiniWebServer_v2: public MiniWebServer {
private:
    ThreadPool threadPool;  // 线程池对象

public:
    explicit MiniWebServer_v2(int poolSize = 0): threadPool(poolSize) {
#ifdef WIN32
        initWSA();
#endif
    }

    void startServer(int port, string ip, int backlog) override;
};





/**
 * 开启 Web Server
 *
 * @param port 监听端口
 * @param ip 本机 ip 地址
 * @param backlog 最大监听 socket 数量
*/
void MiniWebServer_v2::startServer(int port, string ip, int backlog) {
    info(" starting Server...\n")

    //
    // 创建监听 socket，并设置为非阻塞
    //
    SOCKET acceptSocket = createListenSocket(port, backlog, ip);
    unsigned long ul=1;
    ioctlsocket(acceptSocket,FIONBIO, &ul);    //设置成非阻塞模式

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
        int rt = select(0, &readable_fds, NULL, &exceptional_fds,NULL);
        if (rt < 0) {
            info("select failed, Err:%s\n", getErrorInfo().c_str());
            continue;
        } else if (rt == 0) {
            info("select failed, time out\n");
        }

        //
        // 遍历每一个 socket
        //
        unsigned int len = to_be_checked_fds.fd_count;
        for (int i = 0; i < len; i++) {
            //
            // 若该 socket 出现异常
            //
            if (FD_ISSET(to_be_checked_fds.fd_array[i], &exceptional_fds)) {
                info("[socket %s] exceptional socket, Err:%s\n",
                     getSocketIPPort(to_be_checked_fds.fd_array[i]).c_str(), getErrorInfo().c_str());
                continue;
            }

            //
            // 若该 socket 有可读事件
            //
            if (FD_ISSET(to_be_checked_fds.fd_array[i], &readable_fds)) {
                // 若是监听 socket，则接收连接
                if (to_be_checked_fds.fd_array[i] == acceptSocket) {
                    sockaddr connAddr = {};
                    int addrLen = sizeof(connAddr);
                    while (true) {
                        // TODO 队列为空时返回值
                        SOCKET connSocket = accept(acceptSocket, &connAddr, &addrLen);
                        if (connSocket == INVALID_SOCKET) {
                            if (getErrorCode() != WSAEWOULDBLOCK) {
                                err("[socket %s] accept failed, Err:%s\n", getSocketIPPort(connSocket).c_str(),
                                    getErrorInfo().c_str());
                            }
                            break;
                        }
                        ioctlsocket(connSocket,FIONBIO, &ul);    // 设置成非阻塞模式
                        info("[socket %s] new socket\n", getSocketIPPort(connSocket).c_str());

                        // 将新 socket 放入下次 select 集合
                        FD_SET(connSocket, &new_to_be_checked_fds);
                    }
                } else {
                    // 如果是连接 socket，则表明有可读事件，提交任务
                    if (!threadPool.submit(readable_fds.fd_array[i])) {
                        info("[socket %s] submit failed, TaskQueue is full, close socket.\n",
                             getSocketIPPort(readable_fds.fd_array[i]).c_str());
                        closesocket(readable_fds.fd_array[i]);
                    }
                }
                continue;
            }

            //
            // 若该连接 socket 没有任何事件，则放入下一次 select 集合中
            //
            if (to_be_checked_fds.fd_array[i] != acceptSocket) {
                FD_SET(to_be_checked_fds.fd_array[i], &new_to_be_checked_fds);
            }
        }

        //
        // 设置下轮监听集合
        //
        to_be_checked_fds = new_to_be_checked_fds;
        FD_SET(acceptSocket, &to_be_checked_fds);
    }
}

