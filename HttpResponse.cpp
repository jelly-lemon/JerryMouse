#include <winsock2.h>
#include <map>
#include "HttpRequest.cpp"
using namespace std;

class HttpResponse {
private:
    SOCKET connSocket;
    HttpRequest request;
public:
    HttpResponse(SOCKET &connSocket);
    string getRequestData();

    void handleRequest();

    void handleGet();

    void handlePost();
};

/**
 * 获取客户端发送过来的原始字符串
 *
 * @return 客户端发送过来的字符串
 */
string HttpResponse::getRequestData() {
    const int len = 1024;
    char buf[len];
    int code;
    string data;

    while (1) {
        memset(buf, '\0', len);
        code = recv(connSocket, buf, len, 0);
        data += buf;
        if (code == len) {
            // 有可能全部数据刚好 len 个字节，如果再去 recv，就会阻塞
            // 检查是否是全部数据
            if (HttpRequest::isAllData(data))
                break;
            continue;
        } else if (0 < code && code < len) {
            // 数据读完了
        } else if (code == 0) {
            // 客户端主动关闭了
            throw 0;
        } else if (code == SOCKET_ERROR) {
            // socket 异常中断
            throw SOCKET_ERROR;
        }
        break;
    }

    return data;
}


void HttpResponse::handleGet() {

}

void HttpResponse::handlePost() {

}


void HttpResponse::handleRequest() {
    while (1) {
        try {
            // 读取客户端数据
            string data = getRequestData();
            // 构建请求对象
            request = HttpRequest(data);
            if (request.getMethod() == "GET") {
                handleGet();
            } else if (request.getMethod() == "POST") {
                handlePost();
            }
        } catch (int i){
            // 读取客户端数据出错
            break;
        }
    }

    // 关闭连接
    closesocket(connSocket);
}

HttpResponse::HttpResponse(SOCKET &connSocket) {
    this->connSocket = connSocket;
}
