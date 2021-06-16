

#pragma once


#include <fstream>
#include <sstream>

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
        int errorCode = WSAGetLastError();
        char msg[100];
        sprintf_s(msg, "getpeername ERROR:%d", errorCode);
        clientIPport = msg;
    }
    return clientIPport;
}

/**
 * 获取 SOCKET 出错信息
 */
string getWSAErrorInfo() {
    int err_code = WSAGetLastError();
    string err_info("WSAError:");
    switch (err_code) {
        case 10060:
            err_info += "recv timeout";
            break;
        default:
            err_info += to_string(err_code);
            break;
    }

    return err_info;
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
