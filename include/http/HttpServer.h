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
    unsigned int currentConnectionNumber;   // 目前连接数量
    mutex mutexCurrentConnectionNumber;     // 连接数量互斥锁
    unsigned int totalConnectionNumber;     // 累计连接数量
    mutex mutexTotalConnectionNumber;       // 累计连接数量互斥锁

    int port;                       // 监听端口
    string ip;                      // 监听 IP
    int backlog;                    // 全连接队列容量
    unsigned int totalReceivedRequestNumber;        // 累计已收到请求数
    mutex mutexTotalReceivedRequestNumber;                  // 累计已收到请求数互斥锁
    unsigned int totalRespondedRequestNumber;       // 累计已响应请求数
    mutex mutexTotalRespondedRequestNumber;         // 累计已响应请求数互斥锁

    /**
     * 运行服务端
     */
    virtual void run() {

    }

protected:
    SOCKET listenSocket;            // 监听 socket


public:
    explicit HttpServer(int port = 80, string ip = "127.0.0.1", int backlog = 65535) :
            currentConnectionNumber(0), listenSocket(0), port(port), ip(ip),
            backlog(backlog), totalReceivedRequestNumber(0), totalRespondedRequestNumber(0),
            totalConnectionNumber(0) {
#ifdef WIN32
        initWSA();
#endif
        function<void()> recordTask = bind(&HttpServer::recordStatisticInfoBySecond, this);
        thread t(recordTask);
        t.detach();
    }

    ~HttpServer() {
        closeSocket(listenSocket);
    }

    static const string rootDir;  // 资源所在根目录


    /**
     按秒统计请求数据
     */
    void recordStatisticInfoBySecond(){
        unsigned int lastTotalReceivedRequestNumber = totalReceivedRequestNumber;
        unsigned int lastTotalRespondedRequestNumber = totalRespondedRequestNumber;
        ofstream file("./stat.txt", ios::out);
        if (file) {
            info(" create ./stat.txt succeed\n");
        } else {
            info(" create ./stat.txt failed\n");
            return;
        }
        file << "time\\title" << "\tconnection" << "\tRece_delta" << "\tResp_delta" << endl;
        while (1) {
            Sleep(1000);
            file << Logger::getCurrentTime();
            file << "\t" << currentConnectionNumber;
            file << "\t" << totalReceivedRequestNumber - lastTotalReceivedRequestNumber;
            file << "\t" << totalRespondedRequestNumber - lastTotalRespondedRequestNumber;
            file << endl;

            lastTotalReceivedRequestNumber = totalReceivedRequestNumber;
            lastTotalRespondedRequestNumber = totalRespondedRequestNumber;
        }
    }

    /**
     * 获取监听 socket
     */
    SOCKET getAcceptSocket() const {
        return listenSocket;
    }

    /**
     * 累计已响应请求数量 + 1
     */
    void addTotalRespondedRequestNumber() {
        lock_guard<mutex> lockGuard(mutexTotalRespondedRequestNumber);
        totalRespondedRequestNumber++;
    }


    /**
     * 总连接数加 1
     */
    void addTotalReceivedRequestNumber() {
        {
            lock_guard<mutex> lockGuard(mutexTotalReceivedRequestNumber);
            totalReceivedRequestNumber++;
        }
        info(" addTotalReceivedRequestNumber: %d\n", totalReceivedRequestNumber);
    }

    /**
     * 现有连接数量加 1
     */
    void addCurrentConnectionNumber() {
        {
            lock_guard<mutex> guarder(mutexCurrentConnectionNumber);
            currentConnectionNumber++;
            info(" addCurrentConnectionNumber: %d\n", currentConnectionNumber);
        }
        {
            lock_guard<mutex> guarder(mutexTotalConnectionNumber);
            totalConnectionNumber++;
        }
    }

    /**
     * 现有连接数量减 1
     */
    void subCurrentConnectionNumber() {
        lock_guard<mutex> guarder(mutexCurrentConnectionNumber);
        currentConnectionNumber--;
        info(" subCurrentConnectionNumber: %d\n", currentConnectionNumber);
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
            listenSocket = createListenSocket(port, ip);
        } catch (exception &e) {
            err(" --------- startServer failed, Err: %s ---------\n", e.what());
            safeExit(-1);
        }
        info(" --------- Server started ---------\n");

        //
        // 处理连接请求
        //
        info(" waiting for connection...\n");
        run();
    }
};








