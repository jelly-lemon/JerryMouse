#include <winsock2.h>
#include <map>
#include "HttpRequest.cpp"
using namespace std;

class HttpResponse {
private:
    SOCKET connSocket;
    map<string, string> responseHeader;

    static void handleRequest(SOCKET &connSocket);
    static void handleGet(HttpRequest &request);
    static void handlePost(HttpRequest &request);

    static string getRequestData(SOCKET &skt);

public:
    explicit HttpResponse(SOCKET &connSocket): connSocket(connSocket) {

    }

    int write(string &data);


    void addHeader(string &key, string &value);

    string getStrHeader();

    void initDefaultHeader();
};

/**
 * 获取客户端发送过来的原始字符串
 */
string HttpResponse::getRequestData(SOCKET &skt) {
    const int len = 1024;
    char buf[len];
    int code;
    string data;

    while (1) {
        memset(buf, '\0', len);
        code = recv(skt, buf, len, 0);
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


void HttpResponse::handleGet(HttpRequest &request) {

}

void HttpResponse::handlePost(HttpRequest &request) {

}


void HttpResponse::handleRequest(SOCKET &connSocket) {
    while (1) {
        try {
            // 读取客户端数据
            string rawData = getRequestData(connSocket);
            // 构建请求对象
            HttpRequest request(rawData);
            if (request.getMethod() == "GET") {
                handleGet(request);
            } else if (request.getMethod() == "POST") {
                handlePost(request);
            }
        } catch (...){
            // 遇到任何异常就断开连接
            break;
        }
    }

    // 关闭连接
    closesocket(connSocket);
}

int HttpResponse::write(string &data) {
    // 添加 body 长度信息
    string key("Content-Length");
    string value(to_string(data.length()));
    addHeader(key, value);

    string sHeader = getStrHeader();
    string sendData = sHeader + data;
    int n = send(connSocket, &sendData[0], sendData.size(), 0);
    return n;
}

void HttpResponse::addHeader(string &key, string &value) {
    responseHeader[key] = value;
}

string HttpResponse::getStrHeader() {

}

void HttpResponse::initDefaultHeader() {

}

