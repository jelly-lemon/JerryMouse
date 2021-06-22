#ifndef linux
#error only linux support epoll.
#endif
#include <string>
#include <getopt.h>
#include "MiniWebServer.cpp"

using namespace std;

// 基础配置
string HttpResponse::rootDir = "D:/0-3-CLion/MiniWebServer/web_root";   // 资源根目录

/**
 * 打印参数帮助信息
 */
void printUsage() {
    printf("This is a very simple static web server\n");
    printf("version: 1.0\n");

    printf("\nusage:\n");
    printf("%-10s default=0.0.0.0, listen ip address\n", "--ip");
    printf("%-10s default=80, listen port\n", "--port");
    printf("%-10s default=30, max accept socket number\n", "--socket");
    printf("\nexample:\n");
    printf("MiniWebServer.exe --port=12345\n");
    printf("--------------------------------------------------------\n");
}

int main(int argc, char *argv[]) {
    // 默认参数
    printUsage();               // 打印参数使用方法
    string ip = "0.0.0.0";      // 监听 IP
    int port = 80;              // 端口
    int backlog = 99999;         // accept 队列大小
    int theadPoolSize = 8;     // 线程池大小

    // 设置可解析参数列表
    struct option long_options[] =
            {
                    {"ip",      optional_argument,  0, 'i'},
                    {"port",    optional_argument,  0, 'p'},
                    {"backlog", optional_argument,  0, 'b'},
                    {"thead",   optional_argument,  0, 't'},
                    {"help",    no_argument,        0, 'h'},
                    {0,         0,                  0,  0}
            };

    // 对命令行参数进行解析
    while (1) {
        int option_index;   //
        int c = getopt_long_only(argc, argv, "",
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
            case 'b':
                try {
                    backlog = stoi(optarg);
                    break;
                } catch (exception &e) {
                    printf("--socket=%s is invalid\n", optarg);
                    exit(0);
                }
            case 't':
                try {
                    theadPoolSize = stoi(optarg);
                    break;
                } catch (exception &e) {
                    printf("--thead=%s is invalid\n", optarg);
                    exit(0);
                }
            case 'h':
                printUsage();
                exit(0);
            default:
                break;
        }
    }

    // 根据输入的参数启动服务端
    MiniWebServer server(theadPoolSize);
    server.startServer(port, backlog, ip);

    return 0;
}

