#include <iostream>
#include <string>
#include "MiniWebServer.cpp"
#include "HttpRequest.cpp"
using namespace std;

void run_test() {
    MiniWebServer server;
    server.startServer("localhost", 12345);
}



int main() {

    return 0;
}

