#pragma once
#ifdef WIN32
#include "windows.h"
#else
#endif
#include <string>
using namespace std;

/**
 * 相对路径转绝对路径
 *
 * @param relativePath
 * @return
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
