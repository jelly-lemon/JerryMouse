#include <string>
#include <getopt.h>
#include "MiniWebServer.cpp"
#include "HttpResponse.cpp"

using namespace std;

// 基础配置
string HttpResponse::rootDir = "./root";   // 资源根目录




/**
 * 打印参数帮助信息
 */
void printUsage() {
    printf("This is a very simple static web server\n");
    printf("version: 1.0\n");

    printf("\nusage:\n");
    printf("%-10s default=0.0.0.0, listen ip address\n", "--ip");
    printf("%-10s default=80, listen port\n", "--port");
    printf("%-10s default=30, max listen socket number\n", "--socket");
    printf("\nexample:\n");
    printf("MiniWebServer.exe --port=12345\n");
    printf("--------------------------------------------------------\n");
}

int main(int argc, char *argv[]) {
    string ip = "0.0.0.0";
    int port = 80;
    int maxSocketNumber = 30000;

    printUsage();
    // 解析命令行参数
    struct option long_options[] =
            {
                    {"ip",      optional_argument, 0, 'i'},
                    {"port",    optional_argument, 0, 'p'},
                    {"socket",  optional_argument, 0, 's'},
                    {0,         0,                 0, 0}
            };
    while (1) {
        int c;
        int option_index = 0;
        c = getopt_long_only(argc, argv, "",
                             long_options, &option_index);

        if (c == -1) {
            break;
        }
        switch (c) {
            case 'i':
                ip = string(optarg);
                break;
            case 'p':
                try {
                    port = stoi(optarg);
                    break;
                } catch (exception &e) {
                    printf("--port=%s is invalid\n", optarg);
                    exit(0);
                }

            case 's':
                try {
                    maxSocketNumber = stoi(optarg);
                    break;
                } catch (exception &e) {
                    printf("--socket=%s is invalid\n", optarg);
                    exit(0);
                }
        }
    }


    MiniWebServer server;
    server.startServer(port, maxSocketNumber, ip);

    return 0;
}

