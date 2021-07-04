#ifdef WIN32
#error only linux support epoll.
#endif
#include <string>
#include <getopt.h>
#include <csignal>
#include "MiniWebServer_v3.cpp"

using namespace std;

// 基础配置



int main(int argc, char *argv[]) {
    //
    // 默认参数
    //
    bool asyncLog = true;
    bool printInfo = false;
    bool writeToFile = true;
    bool isCleanOldLogFile = true;
    int port = 80;
    string ip = "10.66.38.27";
    int backlog = 65535;
    int poolSize = 0;
//    HttpResponse::rootDir = "D:/0-3-CLion/MiniWebServer_v4/web_root";   // 资源根目录



    //
    // 注册信号处理
    //
    signal(SIGINT, sigHandler);


    //
    // 设置控制台 utf-8 编码
    //
    system("chcp 65001");

    //
    // 启动服务端
    //
    Logger logger(asyncLog, printInfo, writeToFile, isCleanOldLogFile);  // 创建日志对象
    MiniWebServer_v3 server(poolSize);
    server.startServer(port, ip, backlog);

    return 0;
}

