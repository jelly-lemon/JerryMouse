#pragma once
#include <string>
#include "windows.h"
using namespace std;

/**
 * 相对路径转绝对路径
 *
 * @param relativePath
 * @return
 */
string getAbsPath(string relativePath) {
    string absPath;
    char tAbsPath[1024] = {'\0'};
#ifdef WIN32
    if (_fullpath(tAbsPath, relativePath.c_str(), 1024) != NULL) {
        absPath = string(tAbsPath);
    }
#else

#endif
    return absPath;
}
