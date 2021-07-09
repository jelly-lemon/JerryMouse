#pragma once

#include <map>
#include <utility>
#include <map>
#include <SocketHelper.h>
#include "HttpRequest.h"
#include "../CrossPlatform.h"
#include "../util.h"
#include "HttpServer.h"

using namespace std;


/**
 * 对 HTTP 响应封装成类 HttpResponse
 */
class HttpResponse {
private:
    // 响应行
    const string OK_200 = "HTTP/1.1 200 OK\r\n";
    const string NOT_FOUND_404 = "HTTP/1.1 404 Not Found\r\n";

    SOCKET clientSocket;                    // 客户端 socket
    string responseLine;                    // 响应行
    map<string, string> responseHeader;     // 响应头
    string responseBody;                    // 响应体
    HttpRequest httpRequest;                // 客户端请求


    /**
     * 初始化响应头
     * @param header
     */
    void initHeader() {
        responseHeader["Connection"] = "close";
    }

    /**
     * 发送 404 page
     */
    void send404Page() {
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


    /**
     * 向客户端发送数据
     *
     * @param responseLine 响应行
     * @param responseBody 响应体
     * @param contentType 响应体数据类型
     * @return 发送数据的字节数
     */
    int httpSend(const string &responseLine, const string &responseBody, const string &contentType) {
        //
        // 在 Header 中添加 body 类型和长度信息
        //
        addHeader("Content-Length", to_string(responseBody.length()));
        addHeader("Content-Type", contentType);

        //
        // 整理待发送数据
        //
        string sendHeader = getStrHeader();
        string sendData = responseLine + sendHeader + responseBody;
        info("[socket %s] reply\n%s\n", getSocketIPPort(clientSocket).c_str(), sendData.c_str());

        //
        // 发送数据到客户端
        //
        int n = send(clientSocket, sendData.c_str(), sendData.size(), 0);
#ifdef WIN32
        if (n == SOCKET_ERROR) {
            err("send failed, Err:%s\n", getErrorInfo().c_str())
        }
#else

#endif

        return n;
    }

    void handlePost() {
        string contentType("text/plain; charset=UTF-8");
        httpSend(OK_200, "POST request received.", contentType);
    }

    void handleGet() {
        //
        // 判断请求 URL
        //
        string url = httpRequest.getURL();
        if (url == "/") {
            url = "/home.html";         // 默认 / 就是 /home.html
        }
        if (url == "/hello") {
            // 打招呼
            httpSend(OK_200, "Nice to meet you!", "text/plain; charset=UTF-8");
        } else if (url == "/time") {
            // 返回时间
            char msg[101] = {'\0'};
            snprintf(msg, 100, "Server time is:%s\n", Logger::getCurrentTime().c_str());
            httpSend(OK_200, msg, "text/plain; charset=UTF-8");
        } else {
            //
            // 若前面路径都匹配不上，此时尝试根据 URL 读取文件
            //
            try {
                string responseContentType;
                string data = getFile(HttpServer::rootDir + url);
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
                // 向客户端发送数据
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
     * 响应头 map 转字符串
     */
    string getStrHeader() {
        string sendHeader;
        map<string, string>::iterator iter;
        for (iter = responseHeader.begin(); iter != responseHeader.end(); iter++) {
            sendHeader += iter->first + ":" + iter->second + "\r\n";
        }
        sendHeader += "\r\n";

        return sendHeader;
    }

public:
    /**
     * 用 clientSocket 初始化，会自动创建一个 HttpRequest 对象，
     * HttpRequest 会自动完成接收数据的工作
     */
    explicit HttpResponse(SOCKET clientSocket): httpRequest(clientSocket), clientSocket(clientSocket) {
        initHeader();
    }

    /**
     * 用已有的 HttpRequest 对象初始化
     */
    explicit HttpResponse(HttpRequest httpRequest): httpRequest(std::move(httpRequest)) {
        clientSocket = this->httpRequest.getClientSocket();

        initHeader();
    }


    /**
     * 添加响应头
     */
    void addHeader(const string &key, const string &value) {
        if (key.empty())
            return;
        responseHeader[key] = value;
    }



    /**
    * 响应客户端
    */
    void handleRequest() {
        string method = httpRequest.getMethod();
        if (method == "GET") {
            handleGet();
        } else if (method == "POST") {
            handlePost();
        } else {
            info("[socket %s] unknown method: %s\n", getSocketIPPort(clientSocket).c_str(), method.c_str());
            handleGet();
        }
    }
};












