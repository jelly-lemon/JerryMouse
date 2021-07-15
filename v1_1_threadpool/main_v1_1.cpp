#include <string>
#include "HttpServer_v1_1.h"

using namespace std;



using namespace std;

const string HttpServer::rootDir = "../web_root";


int main() {
    Logger logger(true, true, true, true);

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
    HttpServer_v1_1 server(port, ip);
    server.setThreadPool(new ThreadPool(0));
    server.startServer();

    return 0;
}
