#include <iostream>
#include <string>
#include "Log.cpp"
#include "MiniWebServer.cpp"
#include "HttpResponse.cpp"
using namespace std;

// 基础配置
string HttpResponse::rootDir = "D:/0-3-CLion/MiniWebServer/root";   // 资源根目录


int main() {
    MiniWebServer server;
    server.startServer("localhost", 12345, 1);

    return 0;
}

