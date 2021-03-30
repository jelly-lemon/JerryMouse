#include <string>
using namespace std;

class MiniWebServer {
public:
    void startServer(string &ip, int port);

};

/**
 * 处理客户端请求的函数
 */
void* handelResponse(void *args) {
    while(1) {
        // 阻塞等待客户端数据
        if (客户端关闭了连接) {
            break;
        }

        // 解析客户端数据

        // 解析客户端请求

        // 响应请求
        if (发送失败) {
            break;
        }
    }
}

/**
* 开启 Web Server
*
* @param ip 本机 ip 地址
* @param port 监听端口
*/
void MiniWebServer::startServer(string &ip, int port) {
    // 创建监听 socket

    // 等待客户端连接
    while (accept()) {
        // 开启子线程响应请求

    }

}