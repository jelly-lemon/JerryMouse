#include "Reactor.h"
#include "Acceptor.h"
#include "SelectDemultiplexer.h"
using namespace std;


const string HttpServer::rootDir = "../web_root";

int main() {
    Logger logger(true, true, true, true);  // 创建日志对象

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
    Reactor *pReactor = Reactor::getInstance();
    pReactor->setEventDemultiplexer(new SelectDemultiplexer());
    pReactor->registerHandler(new Acceptor(port, ip), EventType::OP_ACCEPT);
    pReactor->handleEvents(1);

    return 0;
}