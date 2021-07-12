#ifndef WIN32
#error only Windows support IOCP.
#endif
#include <string>
#include "HttpServer_v4.h"

#ifdef __GNUC__
#include <getopt.h>
#endif


using namespace std;

const string HttpServer::rootDir = "../web_root";


int main() {
    Logger logger(true, true, true, true);

    int port = 80;
    string ip = "127.0.0.1";


    //
    // 注册信号处理
    //
    signal(SIGINT, sigHandler);


    //
    // 设置 windows 控制台 utf-8 编码
    //
    system("chcp 65001");


    //
    // 启动服务端
    //
    HttpServer_v4 server(port, ip);
    server.startServer();

    return 0;
}
