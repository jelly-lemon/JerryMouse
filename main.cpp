#include <string>
#include "MiniWebServer.cpp"
#include "HttpResponse.cpp"
using namespace std;

// 基础配置
string HttpResponse::rootDir = "D:/0-3-CLion/MiniWebServer/root";   // 资源根目录


void parseArgs(int argc, char *argv[]) {
    // ip default=0.0.0.0，listen ip address

    // port default=80, listen port

    // socket default=30, max listen socket number
}

void printWelcome()  {
    char msg[1025] = {'\0'};

}

int main(int argc, char *argv[]) {

    MiniWebServer server;
    server.startServer(12345, 1, "0.0.0.0");

    return 0;
}

