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
    /**
     * 必须要加 const throw() 才能覆盖父类方法
     * 不然捕获到异常，调用 e.what 只会返回 std::exception
     */
    const char * what() const throw(){
        return msg.c_str();
    }
};