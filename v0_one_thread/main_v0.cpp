
#include "HttpServer_v0.h"

using namespace std;

const string HttpServer::rootDir = "../web_root";

int main() {
    //
    // 设置 windows 控制台 utf-8 编码
    //
    system("chcp 65001");


    //
    // 日志对象
    //
    Logger logger(true, true, true, true);


    //
    // 端口和 IP
    //
    int port = 80;
    string ip = "10.66.38.27";

    //
    // 注册信号处理
    //
    signal(SIGINT, sigHandler);


    //
    // 启动服务端
    //
    HttpServer_v0 server(port, ip);
    server.startServer();

    return 0;
}

