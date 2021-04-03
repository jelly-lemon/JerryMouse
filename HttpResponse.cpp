#include <winsock2.h>
#include <map>
#include <fstream>
#include "HttpRequest.cpp"
using namespace std;

class HttpResponse {
private:
    SOCKET connSocket;
    map<string, string> responseHeader;

    static string rootDir;
    static void handleRequest(SOCKET &connSocket);
    void handleGet(HttpRequest &request);
    static void handlePost(HttpRequest &request);
    static string getRequestData(SOCKET &skt);

public:
    explicit HttpResponse(SOCKET &connSocket): connSocket(connSocket) {
        setRootDir("D:/0-3-CLion/MiniWebServer/root");
    }

    int write(string &data, string contentType);
    void addHeader(string key, string value);
    string getStrHeader();
    void initDefaultHeader();

    static string getFile(string URL);
    static void setRootDir(string dir);

    void send404Page();
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


/**
 * 处理 get 请求
 */
void HttpResponse::handleGet(HttpRequest &request) {
    try {
        string data = getFile(request.getURL());
        // TODO 提取文件类型
        string fileType =
    } catch (...) {
        send404Page();
    }
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

/**
 * 向客户端发送数据
 *
 * @return 发送数据的字节数
 */
int HttpResponse::write(string &data, string contentType) {
    // 添加 body 长度信息
    addHeader("Content-Length", to_string(data.length()));
    addHeader("Content-Type", contentType);

    string sendHeader = getStrHeader();
    string sendData = sendHeader + data;
    int n = send(connSocket, &sendData[0], sendData.size(), 0);
    return n - sendHeader.length();
}

/**
 * 添加响应头
 */
void HttpResponse::addHeader(string key, string value) {
    if (key == "")
        return;
    responseHeader[key] = value;
}

/**
 * 响应头 map 转字符串
 */
string HttpResponse::getStrHeader() {
    string sendHeader;
    map<string, string>::iterator iter;
    for (iter = responseHeader.begin(); iter != responseHeader.end(); iter++) {
        sendHeader += iter->first + ":" + iter->second + "\r\n";
    }
    sendHeader += "\r\n";

    return sendHeader;
}

/**
 * 设置默认响应头
 */
void HttpResponse::initDefaultHeader() {

}

/**
 * 设置资源根目录
 */
void HttpResponse::setRootDir(string dir) {
    rootDir = dir;
}

/**
 * 读取文件
 */
string HttpResponse::getFile(string URL) {
    string filePath = rootDir + URL;
    ifstream file(URL, ios::in);
    if(!file) {
        throw invalid_argument("file not exists");
    } else {
        ostringstream fileContent;
        fileContent << file.rdbuf();
        return fileContent.str();
    }
}

void HttpResponse::send404Page() {
    string data = getFile("/404.html");
    write(data, "text/html");
}
