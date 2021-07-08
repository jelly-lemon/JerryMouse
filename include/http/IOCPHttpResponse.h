// 本文件实现了 IOCPHttpResponse 类
#pragma once

#include <winsock2.h>
#include <map>
#include "util.h"
#include "HttpResponse.h"

using namespace std;


/**
 * IO 操作类型
 *
 * RECV_FINISHED: 读取数据完成
 * SEND_FINISHED: 发送数据完成
 */
enum IO_OPERATION {
    RECV_FINISHED, SEND_FINISHED
};


/**
 * 存储 socket 及 IO 数据
 */
struct IO_DATA {
    OVERLAPPED Overlapped; // 必须为第一个成员
    WSABUF wsabuf;
    IO_OPERATION opCode;
    SOCKET client;
};

/**
 * 对 HTTP 响应封装成类 HttpResponse
 */
class IOCPHttpResponse : public HttpResponse {
private:
    IO_DATA *lpData = NULL;


    int httpSend(const string &responseLine, const string &responseBody, const string &contentType);

public:
    IOCPHttpResponse(IO_DATA *lpData) : HttpResponse(lpData->client),
                                        lpData(lpData) {

    }
};


/**
 * 向客户端发送数据
 *
 * @param responseLine 响应行
 * @param responseBody 响应体
 * @param contentType 响应体数据类型
 * @return 发送数据的字节数
 */
int IOCPHttpResponse::httpSend(const string &responseLine, const string &responseBody, const string &contentType) {
    // 在 Header 中添加 body 类型和长度信息
    setHeader("Content-Length", to_string(responseBody.length()));
    setHeader("Content-Type", contentType);

    // 整理数据
    string sendHeader = getStrHeader();
    string sendData = responseLine + sendHeader + responseBody;

    // 发送数据到客户端
    delete[] lpData->wsabuf.buf;  // 删除接收缓存
    lpData->wsabuf.buf = new char[sendData.length() + 1];
    lpData->wsabuf.len = sendData.length() + 1;
    memset(lpData->wsabuf.buf, '\0', lpData->wsabuf.len);
    strcpy(lpData->wsabuf.buf, sendData.c_str());

    info("[socket %s] reply\n%s\n", getClientIPPort(lpData->client).c_str(), lpData->wsabuf.buf);
    DWORD dwFlags = 0;
    int n = WSASend(lpData->client, &lpData->wsabuf, 1, NULL,
                    dwFlags, &(lpData->Overlapped), NULL);
    if (n == SOCKET_ERROR and WSAGetLastError() != ERROR_IO_PENDING) {
        char msg[1024] = {'\0'};
        snprintf(msg, 1023, "reply failed, %s", getWSAErrorInfo().c_str());
        throw SocketException(msg);
    }

    return n;
}




