#pragma once
#include <winsock2.h>
#include <map>
#include <fstream>
#include <sstream>
#include <iostream>
#include "HttpRequest.cpp"
#include "Exception.cpp"

using namespace std;

class HttpResponse {
private:
    SOCKET connSocket;
    map<string, string> responseHeader;
    string clientIPport;
    const string OK_200 = "HTTP/1.1 200 OK\r\n";
    const string NOT_FOUND_404 = "HTTP/1.1 404 Not Found\r\n";


    void initDefaultHeader();

    void send404Page();

    void handleGet(HttpRequest &request);

    int write(string responseLine, string data, string contentType);

    void addHeader(string key, string value);

    string getStrHeader();

    void handlePost(HttpRequest &request);



    static string getFile(string URL);

    static string getFileType(string url);
    string getClientIPPort();

public:
    static string rootDir;
    explicit HttpResponse(SOCKET &connSocket) : connSocket(connSocket) {

    }
    static string getRequestData(SOCKET &skt);

    void handleRequest();


};

/**
 * 获取客户端发送过来的原始字符串
 */
string HttpResponse::getRequestData(SOCKET &skt) {
    const int len = 1024;
    char buf[len];
    int code;
    string data;

    int recvTimeout = 3 * 1000;
    setsockopt(skt, SOL_SOCKET, SO_RCVTIMEO, (char *)&recvTimeout, sizeof(int));

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
            throw SocketException("client closed connection");
        } else if (code == SOCKET_ERROR) {
            // socket 异常中断或读取超时
            char msg[100];
            sprintf(msg, "SOCKET_ERROR code:%d", WSAGetLastError());
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
    string contentType("text/plain; charset=UTF-8");
    if (url == "/hello" || url == "/") {
        write(OK_200, "Nice to meet you!", contentType);
    } else if (url == "/time") {
        write(OK_200, "Server time is:", contentType);
    } else {
        try {
            string data = getFile(url);
            string fileType = getFileType(url);
            if (fileType == "png" || fileType == "gif" || fileType == "jpeg")
                contentType = "image/" + fileType;
            else if (fileType == "html" || fileType == "plain" || fileType == "xml")
                contentType = "text/" + fileType;
            else
                contentType = "application/octet-stream";
            write(OK_200, data, contentType);
        } catch (invalid_argument e) {
            printf("[tid %d] %s\n", GetCurrentThreadId(), e.what());
            send404Page();
        }
    }
}

/**
 * 根据 url 获取请问的文件类型
 *
 * @param url 请求 url
 * @return 文件类型
 */
string HttpResponse::getFileType(string url) {
    int p = url.find_last_of(".");
    if (p != -1)
        return url.substr(p+1);

    return "";
}

/**
 * 处理 POST 请求
 */
void HttpResponse::handlePost(HttpRequest &request) {
    string url = request.getURL();
    string contentType("text/plain; charset=UTF-8");

    write(OK_200, "received", contentType);
}


/**
 * 处理客户端请求
 */
void HttpResponse::handleRequest() {
    while (1) {
        try {
            // 读取客户端数据
            string rawData = getRequestData(connSocket);
            cout << "[tid " << GetCurrentThreadId() << "]" << "[request " + getClientIPPort() + "]" << endl << rawData << endl;

            // 构建请求对象
            HttpRequest request(rawData);
            if (request.getMethod() == "GET") {
                handleGet(request);
            } else if (request.getMethod() == "POST") {
                handlePost(request);
            }
        } catch (SocketException e) {
            // 遇到任何异常就断开连接
            break;
        }
    }

    // 关闭连接
    closesocket(connSocket);
    cout << "[tid " << GetCurrentThreadId() << "]" << "[close socket " << getClientIPPort() << "]" << "thread finished" << endl;
}

/**
 * 向客户端发送数据
 *
 * @return 发送数据的字节数
 */
int HttpResponse::write(string responseLine, string data, string contentType) {
    // 添加 body 长度信息
    addHeader("Content-Length", to_string(data.length()));
    addHeader("Content-Type", contentType);

    string sendHeader = getStrHeader();
    string sendData = responseLine + sendHeader + data;
    int n = send(connSocket, &sendData[0], sendData.size(), 0);
    if (n == SOCKET_ERROR)
        throw SocketException();

    // 打印发送信息到控制台
    string printSendData;
    if (data.length() > 20)
        printSendData = responseLine + sendHeader + data.substr(0, 20) + "\n...\n";
    else
        printSendData = sendData;
    cout << "[tid " << GetCurrentThreadId() << "]" << "[reply " + getClientIPPort() + "]" << endl << printSendData << endl;
    return n;
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
 * 读取文件
 */
string HttpResponse::getFile(string URL) {
    string filePath = rootDir + URL;
    ifstream file(filePath, ios::in | ios::binary);
    if (!file) {
        throw invalid_argument("file not exists");
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
    string data = getFile("/404.html");
    write(NOT_FOUND_404, data, "text/html");
}

string HttpResponse::getClientIPPort() {
    struct sockaddr_in peerAddr;
    int len = sizeof(peerAddr);
    if (getpeername(connSocket, (struct sockaddr *)&peerAddr, &len) == 0) {
        char info[80];
        sprintf(info, "%s:%d", inet_ntoa(peerAddr.sin_addr), ntohs(peerAddr.sin_port));
        clientIPport = string(info);
        return clientIPport;
    } else
        return clientIPport;
}
