// 本文件实现了 HttpResponse 类
#pragma once

#include <winsock2.h>
#include <map>
#include <sstream>
#include <iostream>
#include "HttpRequest.cpp"
#include "Exception.cpp"
#include "Log.cpp"

using namespace std;


/**
 * 对 HTTP 响应封装成类 HttpResponse
 */
class HttpResponse {
private:
    // 响应行
    const string OK_200 = "HTTP/1.1 200 OK\r\n";
    const string NOT_FOUND_404 = "HTTP/1.1 404 Not Found\r\n";

    SOCKET connSocket;  // 要响应的 socket
    map<string, string> responseHeader; // 响应头
    string clientIPport;    // 客户端 IP和端口

    void send404Page();

    void handleGet(HttpRequest &request);

    int write(const string &responseLine, const string &responseBody, const string &contentType);

    void setHeader(const string &key, const string &value);

    string getStrHeader();

    void initDefaultHeader();

    void handlePost(HttpRequest &request);

    static string getClientIPPort(SOCKET &connSocket);

    static string getFile(const string &URL);

    static string getFileType(string url);

public:
    explicit HttpResponse(SOCKET &connSocket) : connSocket(connSocket) {
        initDefaultHeader();

        clientIPport = getClientIPPort(connSocket);
    }

    void handleRequest();

    string getRequestData(int recvTimeout = 3 * 1000);

    static string rootDir;  // 资源所在根目录
};


/**
 * 获取客户端发送过来的原始字符串
 *
 * @param socket 连接 socket
 * @param recvTimeout 读取超时返回时间
 * @return 客户端发过来的原始字符串
 */
string HttpResponse::getRequestData(int recvTimeout) {
    const int len = 1024;
    char buf[len];
    int code;
    string data;

    // 设置超时返回
    setsockopt(connSocket, SOL_SOCKET, SO_RCVTIMEO, (char *) &recvTimeout, sizeof(int));

    while (1) {
        // 尝试接收 1024 个字节
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
            throw SocketException("client closed connection");
        } else if (code == SOCKET_ERROR) {
            // socket 异常中断或读取超时
            char msg[101] = {'\0'};
            int errorCode = WSAGetLastError();
            if (errorCode == 10060)
                snprintf(msg, 100, "recv timeout");
            else
                snprintf(msg, 100, "recv ERROR code:%d", errorCode);
            throw SocketException(msg);
        }
        break;
    }
    return data;
}


/**
 * 处理 get 请求
 */
void HttpResponse::handleGet(HttpRequest &request) {
    string url = request.getURL();
    string responseContentType("text/plain; charset=UTF-8");

    // 判断请求 URL
    if (url == "/hello" || url == "/") {
        write(OK_200, "Nice to meet you!", responseContentType);
    } else if (url == "/time") {
        write(OK_200, "Server time is:", responseContentType);
    } else {
        // 前面路径都匹配不上，此时尝试根据 URL 读取文件
        try {
            string data = getFile(url);
            string fileType = getFileType(url);
            // 成功读取到文件，并判断文件类型
            if (fileType == "png" || fileType == "gif" || fileType == "jpeg")
                responseContentType = "image/" + fileType;
            else if (fileType == "html" || fileType == "plain" || fileType == "xml")
                responseContentType = "text/" + fileType;
            else
                responseContentType = "application/octet-stream";
            write(OK_200, data, responseContentType);
        } catch (invalid_argument e) {
            // 进入异常说明客户端请求既没有匹配的处理项，也没有对应的文件，就返回 404 页面
            char msg[101] = {'\0'};
            snprintf(msg, 100, "[tid %d] %s\n", GetCurrentThreadId(), e.what());
            Log::record(msg);
            send404Page();
        }
    }
}

/**
 * 根据 url 获取请问的文件类型，如 png。如果 URL 不包含文件类型，返回空串
 *
 * @param url 请求 url
 * @return 文件类型
 */
string HttpResponse::getFileType(string url) {
    int p = url.find_last_of(".");
    if (p != -1)
        return url.substr(p + 1);

    return "";
}

/**
 * 处理 POST 请求
 */
void HttpResponse::handlePost(HttpRequest &request) {
    string url = request.getURL();
    string contentType("text/plain; charset=UTF-8");
    write(OK_200, "POST request received.", contentType);
}


/**
 * 处理客户端请求
 */
void HttpResponse::handleRequest() {
    while (1) {
        try {
            // 读取客户端数据
            string rawData = getRequestData();
            char msg[1024] = {'\0'};
            snprintf(msg, 1023, "[tid %d][request %s]\n%s\n", GetCurrentThreadId(), clientIPport.c_str(), rawData.c_str());
            msg[1022] = '\n';
            Log::record(msg);

            // 构建请求对象，并调用相关方法进行处理
            HttpRequest request(rawData);
            if (request.getMethod() == "GET") {
                handleGet(request);
            } else if (request.getMethod() == "POST") {
                handlePost(request);
            }
        } catch (exception &e) {
            // 遇到任何 socket 异常就跳出循环
            char msg[101] = {'\0'};
            snprintf(msg, 100, "[tid %d][socket %s] %s\n", GetCurrentThreadId(), clientIPport.c_str(), e.what());
            Log::record(msg);
            break;
        }
    }

    // 关闭连接
    closesocket(connSocket);
    char msg[1024] = {'\0'};
    snprintf(msg, 1023, "[tid %d][socket %s] thread finished\n", GetCurrentThreadId(), clientIPport.c_str());
    Log::record(msg);
}

/**
 * 向客户端发送数据
 *
 * @param responseLine 响应行
 * @param responseBody 响应体
 * @param contentType 响应体数据类型
 * @return 发送数据的字节数
 */
int HttpResponse::write(const string &responseLine, const string &responseBody, const string &contentType) {
    // 在 Header 中添加 body 类型和长度信息
    setHeader("Content-Length", to_string(responseBody.length()));
    setHeader("Content-Type", contentType);

    // 发送数据
    string sendHeader = getStrHeader();
    string sendData = responseLine + sendHeader + responseBody;
    int n = send(connSocket, &sendData[0], sendData.size(), 0);
    if (n == SOCKET_ERROR) {
        char msg[1024] = {'\0'};
        snprintf(msg, 1023, "response failed, error code: %d", WSAGetLastError());
        throw SocketException(msg);
    }

    // 打印发送信息到控制台
    string printSendData;
    if (responseBody.length() > 20)
        // 若 responseBody 类容过长，只显示前 20 个字符
        printSendData = responseLine + sendHeader + responseBody.substr(0, 20) + "\n...\n";
    else
        printSendData = sendData;

    char msg[1024] = {'\0'};
    snprintf(msg, 1023, "[tid %d][reply %s]\n%s\n", GetCurrentThreadId(), clientIPport.c_str(), printSendData.c_str());
    Log::record(msg);
    return n;
}

/**
 * 添加响应头
 */
void HttpResponse::setHeader(const string &key, const string &value) {
    if (key.empty())
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
 * 读取文件
 */
string HttpResponse::getFile(const string &URL) {
    string filePath = rootDir + URL;
    ifstream file(filePath, ios::in | ios::binary);     // 二进制模式读取
    if (!file) {
        string msg = URL + " file not exists";
        throw invalid_argument(msg);
    } else {
        ostringstream fileContent;
        fileContent << file.rdbuf();
        return fileContent.str();
    }
}

/**
 * 发送 404 page
 */
void HttpResponse::send404Page() {
    string data = "<!DOCTYPE html>\n"
                  "<html lang=\"en\">\n"
                  "<head>\n"
                  "    <meta charset=\"UTF-8\">\n"
                  "    <title>404 Not Found</title>\n"
                  "</head>\n"
                  "<body>\n"
                  "<h1>Not Found</h1>\n"
                  "<p>The requested URL was not found on this server.</p>\n"
                  "\n"
                  "</body>\n"
                  "</html>";
    write(NOT_FOUND_404, data, "text/html");
}

/**
 * 获取客户端 IP 和端口号
 */
string HttpResponse::getClientIPPort(SOCKET &connSocket) {
    string clientIPport;

    sockaddr_in peerAddr;
    int len = sizeof(peerAddr);
    if (getpeername(connSocket, (struct sockaddr *) &peerAddr, &len) == 0) {
        // socket 存活时才能获取成功
        char info[50];
        sprintf(info, "%s:%d", inet_ntoa(peerAddr.sin_addr), ntohs(peerAddr.sin_port));
        clientIPport = string(info);
    } else {
        // clientIPport 为空串说明服务端尝试读取 socket 时，此 socket 已经被客户端关闭了
        int errorCode = WSAGetLastError();
        char msg[100];
        sprintf_s(msg, "getpeername ERROR:%d", errorCode);
        clientIPport = msg;
    }
    return clientIPport;
}
