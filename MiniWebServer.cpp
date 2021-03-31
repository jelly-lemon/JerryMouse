#include <string>
#include "ThreadPool.cpp"
#include <winsock2.h>
using namespace std;

class MiniWebServer {
public:
    void startServer(string ip, int port);

};



/**
* 开启 Web Server
*
* @param ip 本机 ip 地址
* @param port 监听端口
*/
void MiniWebServer::startServer(string ip, int port) {
    // 创建监听 socket

//    // 等待客户端连接
//    while (accept()) {
//        // 开启子线程响应请求
//
//    }
    ThreadPool pool;
    int i;
    for (i = 0; i < 35; i++) {
        SOCKET  s;
        pool.startThread(s);
    }
}