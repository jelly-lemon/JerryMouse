/**
 * 日志类
 *
 * 主要功能：
 * 输出日志到控制台和日志文件
 */

#pragma once

#include <iostream>
#include <ctime>
#include <pthread.h>
#include <fstream>
#include <io.h>
#include <list>
#include <windows.h>
#include "MsgQueue.cpp"



using namespace std;

class Log {
private:
    static pthread_mutex_t writeLock;   // 写文件互斥锁
    static pthread_mutex_t printLock;   // 打印互斥锁
    static MsgQueue msgQueue;       // 消息队列
    static bool writeThreadIsRunning;
    static string logDir;               // 日志目录

    static bool isWriteToFile;

public:
    void init();

    static void write(string msg, bool printTime = true);

    static void print(ostream *pOut, string msg);

    static void record(string msg, bool printTime = true);

    static string getLogFilePath();

    static string getCurrentTime();

    static void *t_write(void *args);

    static string getString(const char *format, va_list arg);

    static void log(ostream out, const char *format, ...);

    static string getPrefix();

    static string getFormattedStr(const char *format, ...);

    static void log(ostream *out, string &s);

    void startWriteFileThread();
};

/**
 * Info 打印
 */
#define info(...) {\
    string sPrefix = Log::getPrefix();\
    string info = Log::getFormattedStr(__VA_ARGS__);\
    string s = Log::getFormattedStr("%s%s", sPrefix.c_str(), info.c_str());\
    Log::log(&cout, s);\
}

/**
 * TODO Debug 打印
 */
#define debug(...) {\
}

/**
 * Error 打印
 */
#define err(...) {\
    string sPrefix = Log::getPrefix();\
    string lineInfo = Log::getFormattedStr("(%s:%d)", __FILE__, __LINE__);\
    string errInfo = Log::getFormattedStr(__VA_ARGS__);\
    string s = Log::getFormattedStr("%s%s%s", sPrefix.c_str(), lineInfo.c_str(), errInfo.c_str());\
    Log::log(&cerr, s);\
}


/**
 * 初始化日志模块
 */
void Log::init() {
    startWriteFileThread();
}

/**
 * 启动写文件线程
 */
void Log::startWriteFileThread() {

}


/**
 * 线程函数，从消息队列中取消息，然后写入文件
 */
void *Log::t_write(void *args) {
    while (1) {
        // 取数据，若没有数据，阻塞等待
        string msg = msgQueue.get();

        // 获取日志文件路径
        string filePath = getLogFilePath();

        // 写入文件
        // 【易错点】要以二进制格式写入，不然 \r\n 会被写成 \r\r\n
        // 以二进制格式写入，\r\n 就原模原样写入
        ofstream logFile(filePath, ios::app | ios::binary);   // 打开文件
        if (logFile) {
            logFile << msg;
        } else {
            // 文件不存在
            print(&cerr, "文件创建失败:" + filePath);
        }
        logFile.close();    // 关闭文件
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
        pthread_create(&t, NULL, t_write, NULL);
        writeThreadIsRunning = true;
    }

    // 添加信息到消息队列
    pthread_mutex_lock(&writeLock);
    msgQueue.push_back(msg); // 添加到消息队列
    pthread_mutex_unlock(&writeLock);
}

/**
 * 输出到控制台
 */
void Log::print(ostream *pOut, string msg) {
    string sPrefix = getPrefix();

    // 输出到控制台
    *pOut << sPrefix << msg << std::flush;  // 刷新缓冲区
}

/**
 * 获取格式化后的字符串
 *
 * @param format 格式
 * @param ... 参数
 */
string Log::getFormattedStr(const char* format, ...) {
    va_list arg;
    va_start(arg, format);
    string s = getString(format, arg);
    va_end(arg);

    return s;
}

/**
 * 获取格式化后的字符串，代码中做了长度限制，格式化后的字符串最长 1000
 *
 * @param format 格式
 * @param arg 变参指针
 */
string Log::getString(const char *format, va_list arg) {
    int done;
    const int len = 1024; // 字符数组长度
    char msg[len];

    // 格式化
    // 【易错点】要用 vsnprintf，不能用 snprintf
    // 带 v 的都是使用变量 va_list 传递变参
    // 如果格式化后的字符串长度超过 10，则会被截断，并返回 -1
    // 减 4 表示给省略号和\0的位置，即:"...\0"的位置
    done = vsnprintf(msg, len-4, format, arg);

    // 字符串长度超过 len-4 时，最后面省略号表示
    if (done == -1) {
        for (int i = 1; i <= 3; i++) {
            msg[len-1-i] = '.';
        }
        msg[len-1] = '\0';
    } else {
        msg[done] = '\0';
    }

    string formattedStr = string(msg);
    return formattedStr;
}

void Log::log(ostream *out, string &s) {
    pthread_mutex_lock(&printLock);
    *out << s << std::flush;;
    pthread_mutex_unlock(&printLock);

    // 写入文件
    if (isWriteToFile) {
        write(s);
    }
}

/**
 * 打日志
 *
 * @param out 输出对象，cout / cerr
 * @param format 格式
 * @param ... 参数
 */
void Log::log(ostream out, const char *format, ...) {
    // 获取格式化后的字符串
    va_list arg;
    va_start(arg, format);
    string s = getString(format, arg);
    va_end(arg);

    // 打印字符串
    string sPrefix = getPrefix();
    out << sPrefix << s << std::flush;;

    // 写入文件
    if (isWriteToFile) {
        write(s);
    }
}

/**
 * 获取日志前缀
 */
string Log::getPrefix() {
    // 时间
    string sTime = "[" + getCurrentTime() + "]";

    // 线程 id
    char sTid[21];
    snprintf(sTid, 20, "[tid %d]", GetCurrentThreadId());

    string s = sTime + sTid;
    return s;
}

/**
 * 输出到控制台和日志文件
 */
void Log::record(string msg, bool printTime) {
//    return;
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
    int nRet = CreateDirectory(logDir.c_str(), NULL);  // 创建目录（该函数不可递归，只能创建终极目录）
    if (nRet == 0) {
        err("CreateDirectory: %s failed\n", logDir.c_str());
    }

    // 返回日志文件路径
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
    time(&t);           // t = time(NULL) 也可以
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
list <string> Log::msgQueue;
bool Log::writeThreadIsRunning = false;
bool Log::isWriteToFile = false;