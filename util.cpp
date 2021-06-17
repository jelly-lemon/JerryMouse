

#pragma once


#include <fstream>
#include <sstream>
#include "Log.cpp"

/**
 * 获取 CPU 物理核心数
 */
int get_physic_cores() {

    return -1;
}

/**
 * 获取 CPU 逻辑核心数
 */
int get_logic_cores() {

    return -1;
}



/**
 * 获取 SOCKET 出错信息
 */
string getWSAErrorInfo() {
    int err_code = WSAGetLastError();
    string err_info("WSAError:");
    switch (err_code) {
        case WSAEADDRINUSE:
            err_info += "port is in use, can't bind";
            break;
        case WSAENOTSOCK:
            err_info += "Socket operation on nonsocket";
            break;
        case WSAETIMEDOUT:
            err_info += "recv timeout";
            break;
        default:
            err_info += to_string(err_code);
            break;
    }

    return err_info;
}

/**
 * 获取客户端 IP 和端口号
 */
string getClientIPPort(SOCKET &connSocket) {
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
        clientIPport = "getClientIPPort failed, " + getWSAErrorInfo();
    }

    return clientIPport;
}


/**
 * 初始化 socket 调用环境
 */
void initWSA(int a=2, int b=2) {
    WORD wVersionRequested; // WORD 就是 unsigned short，无符号短整型
    WSADATA wsaData;    // 一个结构体。这个结构被用来存储被 WSAStartup 函数调用后返回的 Windows Sockets 数据。
    wVersionRequested = MAKEWORD(a, b); // 将两个 byte 型合并成一个 word 型,一个在高8位(b),一个在低8位(a)。整数 1 是 byte 类型吗？

    // 初始化
    do {
        // 初始化套接字环境
        int n = WSAStartup(wVersionRequested, &wsaData);  // 即WSA(Windows Sockets Asynchronous，Windows异步套接字)的启动命令
        if (n != 0) {
            break;
        }

        // 检查是否初始化成功
        if (LOBYTE(wsaData.wVersion) != a || HIBYTE(wsaData.wVersion) != b) {
            break;
        }
        return;
    } while(0);

    // 若初始化失败
    WSACleanup();   // 功能是终止 Winsock 2 DLL (Ws2_32.dll) 的使用
    err("WSAStartup failed, %s\n", getWSAErrorInfo().c_str())
    exit(-1);
}


/**
 * 读取文件
 */
string getFile(const string &URL) {
    string filePath = URL;
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
 * 根据 url 获取请问的文件类型，如 png
 *
 * @param url 请求 url
 * @return 文件类型
 */
string getFileType(string url) {
    int p = url.find_last_of(".");
    if (p != -1)
        return url.substr(p + 1);
    else
        throw runtime_error("getFileType failed, url:" + url);
}
