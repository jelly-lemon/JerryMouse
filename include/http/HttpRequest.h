// 本文件实现了 HttpRequest 类

#pragma once

#include <map>
#include <string>
#include <stdexcept>
#include <winsock2.h>
#include "util.h"

using namespace std;

/**
 * Http 请求类，把 Http 请求封装成一个类
 */
class HttpRequest {
private:
    SOCKET clientSocket;                // 客户端 socket
    map<string, string> requestLine;    // 请求行
    map<string, string> requestHeader;  // 请求头
    string requestBody;                 // 请求体
    string requestRawData;                     // 客户端请求原始字符串，也就是服务端收到的原始字符串

    void parseRawData();

    static map<string, string> reqLineToMap(const string &reqLine);

    static map<string, string> reqHeaderToMap(const string &reqHeader);

public:
    // 如果一个构造器是接收单个参数的，要加上 explicit
    // 如果不加的话，该构造函数还会拥有类型转换的情形，造成混淆
    /**
     *
     *
     * @param rawData 原始字符串
     */
    explicit HttpRequest(string &rawData, SOCKET clientSocket) :
    requestRawData(rawData), clientSocket(clientSocket) {
        // 如果形参是引用的话，不存在参数为 NULL 的情况，即使强制传 NULL，编译器也不通过，因为类型不匹配
        parseRawData();
    }

    explicit HttpRequest(SOCKET clientSocket): clientSocket(clientSocket){
        requestRawData = recvData();
        parseRawData();
    }


    string getRequestRawData() {
        return requestRawData;
    }





    /**
     * 获取客户端发送过来的原始字符串
     *
     * @param socket 连接 socket
     * @param recvTimeout 读取超时返回时间，单位是毫秒
     * @return 客户端发过来的原始字符串
     */
    string recvData(int recvTimeout = 0) {
        const int len = 1024;
        char buf[len];
        int code;
        string data;

        // 设置超时返回
        // 【易错点】如果超时返回设置 0 的话，就表示一直等待直到有数据
        if (recvTimeout >= 0) {
            setsockopt(clientSocket, SOL_SOCKET, SO_RCVTIMEO, (char *) &recvTimeout, sizeof(int));
        }

        // 读取数据
        while (1) {
            // 尝试接收 1024 个字节
            memset(buf, '\0', len);
            code = recv(clientSocket, buf, len, 0);
            data += buf;

            // 判断读取情况
            char msg[101] = {'\0'};
            // 判断是否收到全部数据
            if (isAllData(data)) {
                break;
            } else if (code == 0) {
                // 客户端主动关闭了
                snprintf(msg, 100, "socket %d closed connection", clientSocket);
                throw runtime_error(msg);
            } else if (code == SOCKET_ERROR) {
                string errInfo = getErrorInfo();
                throw runtime_error(errInfo);
            }
        }

        info("[socket %s] request\n%s\n", getSocketIPPort(clientSocket).c_str(), data.c_str());
        return data;
    }


    string getURL();

    string getMethod();

    string getHttpVersion();

    string getHeader(string key);

    string getBody();

    SOCKET getClientSocket() const {
        return clientSocket;
    }

    static bool isAllData(const string &data);
};


/**
 * 请求行字符串转 map
 *
 * 比如："GET / HTTP/1.1" （字符串后面没有 \r\n）
 * 转为键值对：
 *      method = "GET"
 *      URL = "/"
 *      HttpVersion = "HTTP/1.1"
 *
 * @param reqLine 请求行字符串
 * @return 请求 map
 */
map<string, string> HttpRequest::reqLineToMap(const string &reqLine) {
    int pRequestMethod, pRequestURL;
    map<string, string> reqLineMap;

    // 请求方法
    pRequestMethod = reqLine.find(' ');
    if (pRequestMethod == -1)
        throw invalid_argument("request line data can't be resolved to map\n");
    reqLineMap["method"] = reqLine.substr(0, pRequestMethod);

    // 请求 URL
    pRequestURL = reqLine.find(' ', pRequestMethod + 1);
    if (pRequestURL == -1)
        throw invalid_argument("request line data can't be resolved to map\n");
    reqLineMap["url"] = reqLine.substr(pRequestMethod + 1, pRequestURL - pRequestMethod - 1);

    // HTTP 版本
    reqLineMap["version"] = reqLine.substr(pRequestURL + 1, reqLine.length() - pRequestURL - 1);

    return reqLineMap;
}

/**
 * 请求头字符串转 map
 *
 * @param reqHeader 请求头字符串
 * @return 请求头 map
 */
map<string, string> HttpRequest::reqHeaderToMap(const string &reqHeader) {
    int pMiddle, pEnd, pStart;
    map<string, string> reqHeaderMap;

    pStart = 0;
    do {
        pMiddle = reqHeader.find(":", pStart);
        if (pMiddle == -1)
            throw invalid_argument("request header data can't be resolved to map");
        pEnd = reqHeader.find("\r\n", pStart);
        string key(reqHeader, pStart, pMiddle - pStart);
        string value;
        if (pEnd != -1)
            value = string(reqHeader, pMiddle + 1, pEnd - pMiddle - 1);
        else
            value = string(reqHeader, pMiddle + 1);
        value.erase(0, value.find_first_not_of(' '));   // 去除前面多余空格
        reqHeaderMap[key] = value;
        pStart = pEnd + 2;  // \r\n == 2 个字节
    } while (pEnd != -1);

    return reqHeaderMap;
}


/**
 * 对请求字符串进行解析
 */
void HttpRequest::parseRawData() {
    //
    // 判断内容是否合法
    //
    int headerEndPos, pStart, pEnd;
    headerEndPos = requestRawData.find("\r\n\r\n");   // 找到请求头结束位置
    if (headerEndPos == -1)
        throw invalid_argument("request raw data is not legal\n");

    //
    // 提取请求行
    //
    pStart = 0;
    pEnd = requestRawData.find("\r\n");
    string reqLine(requestRawData, 0, pEnd - pStart);
    requestLine = HttpRequest::reqLineToMap(reqLine);    // 请求行转键值对

    //
    // 提取请求头
    //
    pStart = pEnd + 2;  // \r\n 两个字节
    pEnd = requestRawData.find("\r\n\r\n", pStart);
    if (pEnd != -1) {
        string reqHeader(requestRawData, pStart, pEnd - pStart);
        requestHeader = HttpRequest::reqHeaderToMap(reqHeader);
    }

    //
    // 提取请求体
    //
    int lineAndHeaderLength = headerEndPos + 4; // \r\n\r\n 占 4 个字节，不要用 sizeof("\r\n\r\n")，用 sizeof 会得到 5
    int rawDataLength = requestRawData.length();
    if (rawDataLength == lineAndHeaderLength) {
        // 如果数据长度刚好等于在请求头结束位置，那说明没 body
        requestBody = "";
    } else {
        requestBody = requestRawData.substr(lineAndHeaderLength);
    }
}

/**
 * 判断数据是否收取完毕
 *
 * @param data 目前收到的数据
 */
bool HttpRequest::isAllData(const string &data) {
    //
    // 如果存在 \r\n\r\n，说明请求行和请求头已经发送过来了
    //
    if (data.find("\r\n\r\n") != -1) {
        int pStart = data.find("Content-Length");
        if (pStart != -1) {
            // 提取 Content-Length 对应数值
            pStart = data.find(":", pStart) + 1;
            int pEnd = data.find("\r\n", pStart);
            string sLength(data, pStart, pEnd - pStart);
            int length = stod(sLength); // 尝试转为整数
            pStart = data.find("\r\n\r\n") + 4;
            string body(data, pStart);
            if (body.length() == length)
                return true;
            else
                return false;

        } else {
            return true;
        }
    }
    return false;
}

/**
 * 获取请求 URL，比如 /、/home
 */
string HttpRequest::getURL() {
    return requestLine["url"];
}

/**
 * 获取请求方法，比如 GET、POST
 */
string HttpRequest::getMethod() {
    return requestLine["method"];
}

/**
 * 获取 HTTP 版本，比如 HTTP/1.1
 */
string HttpRequest::getHttpVersion() {
    return requestLine["version"];
}


/**
 * 获取请求头 value，如果不存在则返回空串
 *
 * @param key 请求头的 key
 * @return 请求头的 value
 */
string HttpRequest::getHeader(string key) {
    if (requestHeader.find(key) != requestHeader.end())
        return requestHeader[key];
    else
        return "";
}

/**
 * 获取请求体
 */
string HttpRequest::getBody() {
    return requestBody;
}



