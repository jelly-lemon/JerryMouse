#ifndef WIN32
#error only Windows support IOCP.
#endif
#include <string>
#include <getopt.h>
#include "MiniWebServer.cpp"


using namespace std;

// 基础配置
string HttpResponse::rootDir = "D:/0-2-CLion/MiniWebServer/web_root";   // 资源根目录



int main(int argc, char *argv[]) {
    // 默认参数
//    printUsage();               // 打印参数使用方法
    string ip = "0.0.0.0";      // 监听 IP
    int port = 80;              // 端口
    int backlog = 99999;         // accept 队列大小

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
            case 'h':
                exit(0);
            default:
                break;
        }
    }


    Logger(false, true);

    // 根据输入的参数启动服务端
    MiniWebServer server;
    server.startServer(port, backlog, ip);

    return 0;
}

