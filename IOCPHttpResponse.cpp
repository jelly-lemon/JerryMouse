// 本文件实现了 IOCPHttpResponse 类
#pragma once

#include <winsock2.h>
#include <map>
#include "Exception.cpp"
#include "util.cpp"
#include "BaseHttpResponse.cpp"

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
    DWORD nBytes;
    IO_OPERATION opCode;
    SOCKET client;
};

/**
 * 对 HTTP 响应封装成类 HttpResponse
 */
class IOCPHttpResponse : public BaseHttpResponse {
private:
    IO_DATA *lpIOContext = NULL;


    int httpSend(const string &responseLine, const string &responseBody, const string &contentType);

public:
    IOCPHttpResponse(IO_DATA *lpIOContext) : BaseHttpResponse(lpIOContext->client),
                                             lpIOContext(lpIOContext) {

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
    DWORD dwFlags = 0;
    int n = WSASend(lpIOContext->client, &lpIOContext->wsabuf, 1, &lpIOContext->nBytes,
                    dwFlags, &(lpIOContext->Overlapped), NULL);
    if (n == SOCKET_ERROR and WSAGetLastError() != ERROR_IO_PENDING) {
        char msg[1024] = {'\0'};
        snprintf(msg, 1023, "response failed, %s", getWSAErrorInfo().c_str());
        throw SocketException(msg);
    }

    return n;
}




