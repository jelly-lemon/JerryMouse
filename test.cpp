#include<iostream>
#include "common/Logger.cpp"
using namespace std;

int main() {
    string s = Logger::getFormattedStr("hello%s, %s\n", "123");

    return 0;
}