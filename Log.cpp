/**
 * 日志类
 *
 * 主要功能：输出日志到控制台和日志文件
 */

#pragma once

#include <iostream>
#include <ctime>
#include <pthread.h>    // 操作系统提供的头文件
#include <fstream>
#include <io.h>
// #include <fileapi.h>
#include <list>
#include <windows.h>

using namespace std;

class Log {
private:
    static pthread_mutex_t writeLock;   // 写文件互斥锁
    static pthread_mutex_t printLock;   // 打印互斥锁
    static list <string> msgList;       // 消息队列
    static bool writeThreadIsRunning;

public:
    static void write(string msg, bool printTime = true);

    static void debug();

    static void print(string msg, bool printTime = true);

    static void record(string msg, bool printTime = true);

    static string getLogFilePath();

    static string getCurrentTime();

    static void *t_write(void *args);
};


/**
 * 线程函数，从消息队列中取消息，然后写入文件
 */
void *Log::t_write(void *args) {
    while (1) {
        // 判断当前消息队列是否有消息
        if (msgList.size() > 0) {
            // 获取队首元素
            string msg = msgList.front();
            msgList.pop_front();
            string filePath = getLogFilePath();
            // 【难点】要以二进制格式写入，不然 \r\n 会被解析成 \r\r\n
            // 以二进制格式写入，\r\n 就原模原样写入
            ofstream logFile(filePath, ios::app | ios::binary);   // 打开文件
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
 *
 * @param msg: 信息
 * @param printTime: 是否在信息前打印时间
 */
void Log::write(string msg, bool printTime) {
    // 如果要打印时间，就在信息前添加时间
    if (printTime) {
        string sTime = "[" + getCurrentTime() + "]";
        msg = sTime + msg;
    }

    // 判断写线程是否在运行，如果没有，则启动
    if (!writeThreadIsRunning) {
        pthread_t t;
        int n = pthread_create(&t, NULL, t_write, NULL);
        cout << n << endl;
        writeThreadIsRunning = true;
    }

    // 添加信息到消息队列
    pthread_mutex_lock(&writeLock);
    msgList.push_back(msg); // 添加到消息队列
    pthread_mutex_unlock(&writeLock);
}

/**
 * 输出到控制台
 *
 * @param msg: 信息
 * @param printTime: 是否打印时间
 */
void Log::print(string msg, bool printTime) {
    // 添加时间标记
    if (printTime) {
        string sTime = "[" + getCurrentTime() + "]";
        msg = sTime + msg;
    }

    // 输出到控制台
    pthread_mutex_lock(&printLock);
    cout << msg << std::flush;  // 刷新缓冲区
    pthread_mutex_unlock(&printLock);
}

void Log::debug() {

}

/**
 * 输出到控制台和日志文件
 */
void Log::record(string msg, bool printTime) {
    // 添加时间标记
    if (printTime) {
        string sTime = "[" + getCurrentTime() + "]";
        msg = sTime + msg;
    }

    // 输出到控制台和日志文件
    print(msg, false);
    write(msg, false);
}


/**
 * 获取日志文件路径
 *
 * 日志文件命名方式：年_月_日.txt，如 2021_03_09.txt
 * 换言之，每天都会创建一个日志文件
 */
string Log::getLogFilePath() {
    // 获取当前日期来确定日志文件名
    char fileName[100];
    time_t t;
    struct tm *lt;
    time(&t);  // 获取当前时间，可以精度到秒
    lt = localtime(&t); // 用本地时区表示
    //【易错点】lt->tm_mon 从 0 开始，所以用的时候要加 1
    sprintf(fileName, "%d_%02d_%02d.txt", lt->tm_year+1900, lt->tm_mon+1, lt->tm_mday);

    // 创建日志目录
    string logDir("./log");
    CreateDirectory(logDir.c_str(), NULL);  // 创建目录（该函数不可递归，只能创建终极目录）

    // 返回日志文件路径
    string logPath = logDir + "/" + fileName;
    return logPath;
}

/**
 * 获取当前时间
 *
 * @return 当前时间，字符串
 */
string Log::getCurrentTime() {
    char s[100];
    time_t t;
    struct tm *lt;

    // 获取当前时间，精度到秒
    time(&t); // t = time(NULL) 也可以
    lt = localtime(&t); // 转为本时区时间

    // 格式化时间
    sprintf_s(s, "%d/%02d/%02d %02d:%02d:%02d", lt->tm_year + 1900, lt->tm_mon, lt->tm_mday,
              lt->tm_hour, lt->tm_min, lt->tm_sec);
    return s;
}

/**
 * 静态变量初始化（只能在类外部）
 */
pthread_mutex_t Log::writeLock;
pthread_mutex_t Log::printLock;
list <string> Log::msgList;
bool Log::writeThreadIsRunning = false;