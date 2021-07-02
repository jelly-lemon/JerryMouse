#pragma once

#include <map>
#include "HttpRequest.cpp"
#include "CrossPlatform.h"
#include "util.cpp"

using namespace std;


/**
 * 对 HTTP 响应封装成类 HttpResponse
 */
class HttpResponse {
private:
    //
    // 响应行
    //
    const string OK_200 = "HTTP/1.1 200 OK\r\n";
    const string NOT_FOUND_404 = "HTTP/1.1 404 Not Found\r\n";

    SOCKET connSocket;                  // 要响应的 socket
    map<string, string> responseHeader; // 响应头
    string clientIPport;                // 客户端 IP 和端口


    void send404Page();

    virtual int httpSend(const string &responseLine, const string &responseBody, const string &contentType);

    void handlePost(HttpRequest &request);

    void handleGet(HttpRequest &request);


protected:
    void setHeader(const string &key, const string &value);

    string getStrHeader();

public:
    explicit HttpResponse(SOCKET &connSocket) : connSocket(connSocket) {
        clientIPport = getSocketIPPort(connSocket);
    }

    void handleRequest(string rawData = "");

    string getRequestData(int recvTimeout = 0);

    static string rootDir;  // 资源所在根目录

    void handleRequest(HttpRequest &request);

    static int closeSocket(SOCKET &connSocket);

};

//
// 静态变量初始化
//
string HttpResponse::rootDir;


/**
 * 获取客户端发送过来的原始字符串
 *
 * @param socket 连接 socket
 * @param recvTimeout 读取超时返回时间，单位是毫秒
 * @return 客户端发过来的原始字符串
 */
string HttpResponse::getRequestData(int recvTimeout) {
    const int len = 1024;
    char buf[len];
    int code;
    string data;

    // 设置超时返回
    // 【易错点】如果超时返回设置 0 的话，就表示一直等待直到有数据
    if (recvTimeout >= 0) {
        setsockopt(connSocket, SOL_SOCKET, SO_RCVTIMEO, (char *) &recvTimeout, sizeof(int));
    }

    // 读取数据
    while (1) {
        // 尝试接收 1024 个字节
        memset(buf, '\0', len);
        code = recv(connSocket, buf, len, 0);
        data += buf;

        // 判断读取情况
        char msg[101] = {'\0'};
        if (code == 0) {
            // 客户端主动关闭了
            snprintf(msg, 100, "socket %d closed connection", connSocket);
            throw runtime_error(msg);
        } else if (code == SOCKET_ERROR) {
            // socket 异常中断或读取超时
            int errorCode = getErrorCode();
            if (errorCode == 10060)
                snprintf(msg, 100, "recv timeout");
            else if (errorCode == 10035) {
                snprintf(msg, 100, " recv buffer is empty");
                closesocket(connSocket);
            } else
                snprintf(msg, 100, "socket %d recv failed, Err: %s", connSocket, getErrorInfo().c_str());
            throw runtime_error(msg);
        } else {
            // 判断是否收到全部数据
            if (HttpRequest::isAllData(data))
                break;
        }
    }

    return data;
}


/**
 * 处理 get 请求
 */
void HttpResponse::handleGet(HttpRequest &request) {
    //
    // 默认响应类型
    //
    string responseContentType("text/plain; charset=UTF-8");

    //
    // 判断请求 URL
    //
    string url = request.getURL();
    if (url == "/") {
        url = "/home.html";         // 默认 / 就是 /home.html
    }
    if (url == "/hello") {
        // 打招呼
        httpSend(OK_200, "Nice to meet you!", responseContentType);
    } else if (url == "/time") {
        // 返回时间
        char msg[101] = {'\0'};
        snprintf(msg, 100, "Server time is:%s\n", Logger::getCurrentTime().c_str());
        httpSend(OK_200, msg, responseContentType);
    } else {
        //
        // 若前面路径都匹配不上，此时尝试根据 URL 读取文件
        //
        try {
            string data = getFile(rootDir + url);
            string fileType = getFileType(url);
            // 如果是图片
            if (fileType == "png" || fileType == "gif" || fileType == "jpeg" || fileType == "svg") {
                if (fileType == "svg")
                    responseContentType = "image/svg+xml";
                else
                    responseContentType = "image/" + fileType;
            } else if (fileType == "html" || fileType == "plain" || fileType == "xml" || fileType == "css") {
                // 如果是 html 文件
                responseContentType = "text/" + fileType;
            } else {
                responseContentType = "application/octet-stream";
            }

            //
            // 进行响应
            //
            httpSend(OK_200, data, responseContentType);
        } catch (exception &e) {
            //
            // 找不到匹配项，则返回 404 页面
            //
            send404Page();
        }
    }
}


/**
 * 处理 POST 请求
 */
void HttpResponse::handlePost(HttpRequest &request) {
    string url = request.getURL();
    string contentType("text/plain; charset=UTF-8");
    httpSend(OK_200, "POST request received.", contentType);
}

/**
 * 处理客户端请求
 *
 * @param request: 客户端请求
 */
void HttpResponse::handleRequest(HttpRequest &request) {
    if (request.getMethod() == "GET") {
        handleGet(request);
    } else if (request.getMethod() == "POST") {
        handlePost(request);
    }
}

/**
 * 处理客户端请求
 *
 * @param rawData: 请求内容（原始字符串形式）。若 rawData 为空串，则会调用 recv 尝试读取
 */
void HttpResponse::handleRequest(string rawData) {
    do {
        //
        // 若 rawData 为空，则尝试读取
        //
        if (rawData.empty()) {
            try {
                rawData = getRequestData();   // 读取客户端数据
            } catch (exception &e) {
                err("[socket %s] getRequestData failed, Err:%s\n",
                    clientIPport.c_str(),
                    e.what());
                break;
            }
        }
        info("[socket %s] request\n%s\n", clientIPport.c_str(), rawData.c_str())

        //
        // 响应请求
        //
        try {
            // 构建请求对象，并调用相关方法进行处理
            HttpRequest request(rawData);
            handleRequest(request);
        } catch (exception &e) {
            err("[socket %s] handleRequest failed, Err:%s\n", clientIPport.c_str(), e.what())
            break;
        }
    } while (0);

    //
    // 关闭连接
    //
    HttpResponse::closeSocket(connSocket);
}

/**
 * 关闭 socket
 *
 * @param connSocket
 * @return
 */
int HttpResponse::closeSocket(SOCKET &connSocket) {
    string strIpPort = getSocketIPPort(connSocket);
    int n = closesocket(connSocket);
    if (n == SOCKET_ERROR) {
        err("[socket %s] close socket %d err, %s\n", strIpPort.c_str(), connSocket, getErrorInfo().c_str());
    } else {
        info("[socket %s] we closed socket %d.\n", strIpPort.c_str(), connSocket);
    }

    return n;
}


/**
 * 向客户端发送数据
 *
 * @param responseLine 响应行
 * @param responseBody 响应体
 * @param contentType 响应体数据类型
 * @return 发送数据的字节数
 */
int HttpResponse::httpSend(const string &responseLine, const string &responseBody, const string &contentType) {
    //
    // 在 Header 中添加 body 类型和长度信息
    //
    setHeader("Content-Length", to_string(responseBody.length()));
    setHeader("Content-Type", contentType);
    setHeader("Connection", "close");   // 短连接

    //
    // 整理待发送数据
    //
    string sendHeader = getStrHeader();
    string sendData = responseLine + sendHeader + responseBody;
    info("[socket %s] reply\n%s\n", clientIPport.c_str(), sendData.c_str());

    //
    // 发送数据到客户端
    //
    int n = send(connSocket, sendData.c_str(), sendData.size(), 0);
    if (n == SOCKET_ERROR) {
        err("send failed, Err:%s\n", getErrorInfo().c_str())
    }

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
    httpSend(NOT_FOUND_404, data, "text/html");
}


