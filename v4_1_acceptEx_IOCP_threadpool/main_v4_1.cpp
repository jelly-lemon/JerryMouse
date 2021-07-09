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
    //
    // 默认参数
    //
    bool isAsyncLog = true;
    bool isPrintInfo = true;
    bool isWriteToFile = true;
    bool isCleanOldLogFile = true;
    int port = 80;
    string ip = "10.66.38.27";
    int backlog = 65535;
    int poolSize = 0;
    string webRoot = "../web_root";


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
    Logger logger(isAsyncLog, isPrintInfo, isWriteToFile, isCleanOldLogFile);  // 创建日志对象
    HttpServer_v4_1 server(poolSize);
    server.startServer(port, ip, backlog);

    return 0;
}

