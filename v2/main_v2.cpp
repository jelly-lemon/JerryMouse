#include "../common/HttpResponse.cpp"
#include "../common/Logger.cpp"
#include "MiniWebServer_v2.cpp"
using namespace std;





int main() {
    //
    // 参数设置
    //
    HttpResponse::rootDir = "../web_root";   // 资源根目录


    //
    // 启动服务端
    //
    Logger logger;  // 创建日志对象
    MiniWebServer_v2 server;
    server.startServer(80, "localhost", 65535);

    return 0;
}

