/**
 * 异常类
 */

#pragma once
#include <iostream>
#include <exception>
using namespace std;


/**
 * 自定义 Socket 异常
 */
struct SocketException : public exception {
private:
    string msg;

public:
    explicit SocketException(string msg = "Socket Error"): msg(msg) {

    }
    const char * what() {
        return &msg[0];
    }
};