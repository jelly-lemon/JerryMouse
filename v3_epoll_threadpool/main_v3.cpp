#ifdef WIN32
#error only linux support epoll.
#endif
#include <string>
#include <getopt.h>
#include <csignal>
#include "CrossPlatform.h"
#include "HttpServer_v3.cpp"

using namespace std;

// 基础配置

const string HttpServer::rootDir = "../web_root";

int main(int argc, char *argv[]) {
    Logger logger(true, true, true, true);  // 创建日志对象
    int port = 80;
    string ip = "192.168.1.5";


    //
    // 注册信号处理
    //
    signal(SIGINT, sigHandler);




    HttpServer_v3 server(port, ip);
    server.startServer();

    return 0;
}

