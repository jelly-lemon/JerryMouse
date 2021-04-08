#pragma once

#include <iostream>
#include <ctime>
#include <pthread.h>    // 操作系统提供的头文件
#include <fstream>
#include <io.h>
#include <fileapi.h>
#include <list>
#include <windows.h>

using namespace std;

class Log {
private:
    static pthread_mutex_t writeLock;   // 写文件互斥锁
    static pthread_mutex_t printLock;   // 打印互斥锁
    static list <string> msgList;    // 消息队列
    static bool writeThreadIsRunning;

public:
    static void write(string msg, bool printTime = true);

    static void print(string msg, bool printTime = true);

    static void record(string msg, bool printTime = true);

    static string getLogFilePath();

    static string getCurrentTime();

    static void* t_write(void* args);
};


/**
 * 线程函数，从消息队列中取消息，然后写入文件
 */
void* Log::t_write(void* args) {
    while (1) {
        if (msgList.size() > 0) {
            // 获取队首元素
            string msg = msgList.front();
            msgList.pop_front();
            string filePath = getLogFilePath();
            ofstream logFile(filePath, ios::app);   // 打开文件
            if (logFile) {
                logFile << msg;
            }
            logFile.close();    // 关闭文件
        } else {
            // 消息队列无内容则挂起 0.1s
            Sleep(100);
        }
    }
}

/**
 * 输出到日志文件
 */
void Log::write(string msg, bool printTime) {
    if (printTime) {
        string sTime = "[" + getCurrentTime() + "]";
        msg = sTime + msg;
    }
    if (!writeThreadIsRunning) {
        pthread_t t;
        pthread_create(&t, NULL, t_write, NULL);
        writeThreadIsRunning = true;
    }
    pthread_mutex_lock(&writeLock);
    msgList.push_back(msg); // 添加到消息队列
    pthread_mutex_unlock(&writeLock);
}

/**
 * 输出到控制台
 */
void Log::print(string msg, bool printTime) {
    if (printTime) {
        string sTime = "[" + getCurrentTime() + "]";
        msg = sTime + msg;
    }
    pthread_mutex_lock(&printLock);
    cout << msg << std::flush;  // 刷新缓冲区
    pthread_mutex_unlock(&printLock);
}

/**
 * 输出到控制台和日志文件
 */
void Log::record(string msg, bool printTime) {
    if (printTime) {
        string sTime = "[" + getCurrentTime() + "]";
        msg = sTime + msg;
    }
    print(msg, false);
    write(msg, false);
}

/**
 * 获取日志文件路径
 */
string Log::getLogFilePath() {
    char fileName[100];
    time_t t;
    struct tm *lt;
    // 获取当前时间，精度到秒
    time(&t);
    lt = localtime(&t);
    sprintf(fileName, "%d_%02d_%02d.txt", lt->tm_year + 1900, lt->tm_mon, lt->tm_mday);

    // TODO 判断文件夹是否存在
    // 创建日志目录
    string logDir("./log");
    CreateDirectory(logDir.c_str(), NULL);

    string logPath = logDir + "/" + fileName;
    return logPath;
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

    sprintf_s(s, "%d/%02d/%02d %02d:%02d:%02d", lt->tm_year + 1900, lt->tm_mon, lt->tm_mday,
              lt->tm_hour, lt->tm_min, lt->tm_sec);
    return s;
}

/**
 * 静态变量初始化
 */
pthread_mutex_t Log::writeLock;
pthread_mutex_t Log::printLock;
list<string> Log::msgList;
bool Log::writeThreadIsRunning = false;