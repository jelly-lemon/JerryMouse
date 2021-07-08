// 本文件实现了 IOCPHttpResponse 类
#pragma once

#include <winsock2.h>
#include <map>
#include "../include/util.h"
#include "../include/http/HttpResponse.h"

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
 *
 * 【注意】OVERLAPPED Overlapped; 必须为第一个成员
 */
struct IO_DATA {
    OVERLAPPED Overlapped;
    WSABUF wsabuf;
    IO_OPERATION opCode;
    SOCKET client;
    DWORD acceptCompletedTime;
    DWORD beginHandleTime;
};

/**
 * 对 HTTP 响应封装成类 HttpResponse
 */
class IOCPHttpResponse : public HttpResponse {
private:
    IO_DATA *pIoData = NULL;

    int httpSend(const string &responseLine, const string &responseBody, const string &contentType);

public:
    IOCPHttpResponse(IO_DATA *pIoData) : HttpResponse(pIoData->client),
                                         pIoData(pIoData) {

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
    //
    // 在 Header 中添加 body 类型和长度信息
    //
    setHeader("Content-Length", to_string(responseBody.length()));
    setHeader("Content-Type", contentType);

    //
    // 整理待发送数据
    //
    string sendHeader = getStrHeader();
    string sendData = responseLine + sendHeader + responseBody;

    //
    // 发送数据到客户端
    //
    //delete[] pIoData->wsabuf.buf;  // 删除接收缓存
    pIoData->wsabuf.buf = new char[sendData.length() + 1];
    pIoData->wsabuf.len = sendData.length() + 1;
    memset(pIoData->wsabuf.buf, '\0', pIoData->wsabuf.len);
    strcpy(pIoData->wsabuf.buf, sendData.c_str());
    info("[socket %s] reply\n%s\n", getSocketIPPort(pIoData->client).c_str(), pIoData->wsabuf.buf);
    DWORD dwFlags = 0;
    int n = WSASend(pIoData->client, &pIoData->wsabuf, 1, NULL,
                    dwFlags, &(pIoData->Overlapped), NULL);
    // ------------------------------
    //
    // ERROR_IO_PENDING 表示：
    //
    //-------------------------------
    if (n == SOCKET_ERROR && WSAGetLastError() != ERROR_IO_PENDING) {
        char msg[1024] = {'\0'};
        snprintf(msg, 1023, "WSASend failed");
        throw runtime_error(msg);
    }

    return n;
}




