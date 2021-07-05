#include <string>
#include "WebServer_v1_1.cpp"

using namespace std;



int main(int argc, char *argv[]) {
    //
    // 默认参数
    //
    bool asyncLog = true;
    bool printInfo = true;
    bool writeToFile = true;
    bool isCleanOldLogFile = true;
    int port = 80;
    string ip = "10.66.38.27";
    int backlog = 65535;
    int poolSize = 0;
    HttpResponse::rootDir = "../web_root";   // 资源根目录



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
    Logger logger(asyncLog, printInfo, writeToFile, isCleanOldLogFile);  // 创建日志对象
    WebServer_v1_1 server(poolSize);
    server.startServer(port, ip, backlog);

    return 0;
}

