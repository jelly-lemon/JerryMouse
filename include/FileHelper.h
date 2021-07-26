#pragma once
#ifdef WIN32
#include "windows.h"
#else
#endif
#include <string>

using namespace std;


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

/**
 * 相对路径转绝对路径
 *
 * @param relativePath 相对路径
 * @return 绝对路径
 */
string getAbsPath(string relativePath) {
    string absolutePath;
    char t[1024] = {'\0'};
#ifdef WIN32
    if (_fullpath(t, relativePath.c_str(), 1024) != NULL) {
        absolutePath = string(t);
    }
#else
    if (realpath(relativePath.c_str(), t) != NULL) {
        absolutePath = string(t);
    }

#endif
    return absolutePath;
}
