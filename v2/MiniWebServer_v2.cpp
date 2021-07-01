#pragma once

#include <winsock2.h>
#include <string>
#include <unordered_map>
#include "../common/MiniWebServer.cpp"
#include "../common/ThreadPool.cpp"

using namespace std;


class MiniWebServer_v2: public MiniWebServer {
private:
    ThreadPool threadPool;  // 线程池对象
    unordered_map<SOCKET, long> acceptedTime;

public:
    explicit MiniWebServer_v2(int poolSize = 0): threadPool(poolSize){
#ifdef WIN32
        initWSA();
#endif
        showUsage();

        //
        // 设置任务完成时回调函数
        //
        function<void()> callback = bind(&MiniWebServer::subConnectionNumber, this);
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
        struct timeval selectTimeOut = {};
        selectTimeOut.tv_sec = 1;
        int rt = select(0, &readable_fds, NULL, &exceptional_fds, &selectTimeOut);
        if (rt < 0) {
            info("select failed, Err:%s\n", getErrorInfo().c_str());
        } else if (rt == 0) {
            info("select time out\n");
        }
        int fd_size = to_be_checked_fds.fd_count;

        //
        // 遍历每一个 socket
        //
        unsigned int len = to_be_checked_fds.fd_count;
        info(" to_be_checked_fds.fd_count: %d\n", to_be_checked_fds.fd_count);
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
                //
                // 若是监听 socket，则接收连接
                //
                if (to_be_checked_fds.fd_array[i] == acceptSocket) {
                    sockaddr connAddr = {};
                    int addrLen = sizeof(connAddr);
                    while (true) {
                        //
                        // 集合最大容量，如果本次集合已满，就不 accept
                        //
                        if (fd_size >= FD_SETSIZE) {
                            break;
                        }


                        //
                        // 接收新连接
                        //
                        SOCKET connSocket = accept(acceptSocket, &connAddr, &addrLen);
                        if (connSocket == INVALID_SOCKET) {
                            // WSAEWOULDBLOCK 表示 accept 队列为空
                            if (getErrorCode() != WSAEWOULDBLOCK) {
                                err("[socket %s] accept failed, Err:%s\n", getSocketIPPort(connSocket).c_str(),
                                    getErrorInfo().c_str());
                                safeExit(-1);
                            }
                            break;
                        }


                        //
                        // 将新 socket 放入下次 select 集合
                        //
                        addConnectionNumber();
                        //ioctlsocket(connSocket,FIONBIO, &ul);    // 设置成非阻塞模式
                        info("[socket %s] new socket\n", getSocketIPPort(connSocket).c_str());
                        FD_SET(connSocket, &new_to_be_checked_fds);
                        fd_size++;
                    }
                } else {
                    //
                    // 如果是连接 socket，则表明有可读事件，提交任务
                    //
                    long waitTime = getTimeDiff(acceptedTime[readable_fds.fd_array[i]]);
                    info("[socket %s] wait time: %d ms\n", getSocketIPPort(readable_fds.fd_array[i]).c_str(), waitTime);
                    if (!threadPool.submitTask(readable_fds.fd_array[i])) {
                        info("[socket %s] submitTask failed, TaskQueue is full, close socket.\n",
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
        if (new_to_be_checked_fds.fd_count < FD_SETSIZE) {
            FD_SET(acceptSocket, &to_be_checked_fds);
        }
    }
}

void MiniWebServer_v2::showUsage() {

}

