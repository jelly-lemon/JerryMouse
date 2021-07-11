#pragma once


#include <string>
#include "CrossPlatform.h"
#include "Logger.h"
#include "util.h"
#include "SocketHelper.h"
#include "FileHelper.h"

#ifdef WIN32
#include <winsock2.h>
#else
#endif
using namespace std;


/**
 * 对服务端封装成类
 */
class HttpServer {
private:
    unsigned int connectionNumber;  // 连接数量
    mutex mutexConnectionNumber;    // 连接数量互斥锁
    int port;                       // 监听端口
    string ip;                      // 监听 IP
    int backlog;                    // 全连接队列容量

    /**
     * 处理 accept
     */
    virtual void handleAccept() = 0;

protected:
    SOCKET acceptSocket;            // 监听 socket


public:
    explicit HttpServer(int port = 80, string ip = "127.0.0.1", int backlog = 65535):
    connectionNumber(0), acceptSocket(0), port(port), ip(ip), backlog(backlog) {
#ifdef WIN32
        initWSA();
#endif
    }

    static const string rootDir;  // 资源所在根目录




    /**
     * 获取当前连接数量
     */
    unsigned int getConnectionNumber() {
        lock_guard<mutex> lockGuard(mutexConnectionNumber);
        return connectionNumber;
    }


    /**
     * 获取监听 socket
     */
    SOCKET getAcceptSocket() const {
        return acceptSocket;
    }

    /**
     * 现有连接数量加 1
     */
    void addConnectionNumber() {
        lock_guard<mutex> guarder(mutexConnectionNumber);
        connectionNumber++;
        info(" addConnectionNumber: %d\n", connectionNumber);
    }

    /**
     * 现有连接数量减 1
     */
    void subConnectionNumber() {
        lock_guard<mutex> guarder(mutexConnectionNumber);
        connectionNumber--;
        info(" subConnectionNumber: %d\n", connectionNumber);
    }

    /**
     * 启动服务端
     *
     * @param isNonBlocking 监听 socket 是否非阻塞
     */
    void startServer() {
        //
        // 启动 server
        //
        info(" --------- starting Server ---------\n")
        info(" main thread tid: %ld\n", getThreadID());
        try {
            info(" web_root dir is %s\n", getAbsPath(HttpServer::rootDir).c_str())
            acceptSocket = createListenSocket(port, ip);
        } catch (exception &e) {
            err(" --------- startServer failed, Err: %s ---------\n", e.what());
            safeExit(-1);
        }
        info(" --------- Server started ---------\n");

        //
        // 处理连接请求
        //
        info(" waiting for connection...\n");
        handleAccept();
    }
};








