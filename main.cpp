#include <iostream>
#include <string>
#include "MiniWebServer.cpp"
#include "HttpResponse.cpp"
using namespace std;

string HttpResponse::rootDir = "D:/0-3-CLion/MiniWebServer/root";

int main() {


    MiniWebServer server;
    server.startServer("localhost", 12345, 30);

    return 0;
}

