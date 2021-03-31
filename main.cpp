#include <iostream>
#include <string>
#include "MiniWebServer.cpp"
using namespace std;


int main() {
    MiniWebServer server;
    server.startServer("localhost", 12345);

    return 0;
}

