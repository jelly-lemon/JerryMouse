#pragma once

#include <winsock2.h>
#include <string>
#include <unordered_map>
#include "../common/HttpServer.cpp"
#include "../common/ThreadPool.cpp"

using namespace std;


class HttpServer_v2_2 : public HttpServer {
private:
    ThreadPool<pair<SOCKET, long>> threadPool;  // 线程池对象
    unordered_map<SOCKET, long> acceptedTime;   // 哈希表，记录socket 被 accepted 时的时间

public:
    explicit HttpServer_v2_2(int poolSize = 0) : threadPool(poolSize) {
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

};


/**
 * 开启 Web Server
 *
 * @param port 监听端口
 * @param ip 本机 ip 地址
 * @param backlog 最大监听 socket 数量
*/
void HttpServer_v2_2::startServer(int port, string ip, int backlog) {
    info(" starting Server...\n")

    //
    // 创建监听 socket，并设置为非阻塞
    //
    SOCKET acceptSocket = createListenSocket(port, backlog, ip);
    unsigned long ul = 1;
    ioctlsocket(acceptSocket, FIONBIO, &ul);    //设置成非阻塞模式
    info(" acceptSocket: %d\n", acceptSocket)
}



