#include <string>
#include "HttpServer_v1_1.h"

using namespace std;



int main(int argc, char *argv[]) {
    Logger logger(true, false, true, true);  // 创建日志对象

    int port = 80;
    string ip = "10.66.38.27";


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
    HttpServer_v1_1 server(poolSize);
    server.startServer(port, ip, backlog);

    return 0;
}

