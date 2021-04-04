#pragma once
#include <iostream>
#include <exception>

using namespace std;

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