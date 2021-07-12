/**
 * 仅支持 Windows
 */
#ifndef WIN32
#error only Windows support IOCP.
#endif
#include <string>
#include <getopt.h>
#include "HttpServer_v4_1.h"
#include "http/HttpServer.h"

using namespace std;

const string HttpServer::rootDir = "../web_root";   // 资源根目录


int main(int argc, char *argv[]) {
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
    HttpServer_v4_1 server(port, ip);
    server.startServer();

    return 0;
}

