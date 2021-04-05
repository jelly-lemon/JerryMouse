#pragma once

#include <iostream>
#include <time.h>
#include <pthread.h>
using namespace std;

class Log {
private:
    static pthread_mutex_t writeLock;
    static pthread_mutex_t printLock;

public:
    Log(const void *writeLock);

    static void write(string msg);
    static void print(string msg);
    static void record(string msg);

    void createLogfile();

    string getCurrentTime();
};

/**
 * 输出到日志文件
 */
void Log::write(string msg) {
    pthread_mutex_lock(&writeLock);

    pthread_mutex_unlock(&writeLock);
}

/**
 * 输出到控制台
 */
void Log::print(string msg) {
    pthread_mutex_lock(&printLock);
    cout << msg;
    pthread_mutex_unlock(&printLock);
}

/**
 * 输出到控制台和日志文件
 */
void Log::record(string msg) {
    print(msg);
    write(msg);
}

/**
 * 创建日志文件
 *
 * @param path 文件路径
 */
void Log::createLogfile() {
    char fileName[100];
    time_t t;
    struct tm *lt;

    // 获取当前时间，精度到秒
    time(&t);
    lt = localtime(&t);//转为时间结构。
    sprintf_s(fileName, "%d_%d_%d.txt", lt->tm_year + 1900, lt->tm_mon, lt->tm_mday);
}

/**
 * 获取当前时间
 */
string Log::getCurrentTime() {
    char s[100];
    time_t t;
    struct tm *lt;

    // 获取当前时间，精度到秒
    time(&t); // t = time(NULL) 也可以
    lt = localtime(&t);//转为时间结构。

    sprintf_s(s, "%d/%d/%d %d:%d:%d", lt->tm_year + 1900, lt->tm_mon, lt->tm_mday,
              lt->tm_hour, lt->tm_min, lt->tm_sec);
    return s;
}

pthread_mutex_t Log::writeLock;
pthread_mutex_t Log::printLock;
