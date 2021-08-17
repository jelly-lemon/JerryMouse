#pragma once

#ifdef WIN32
#include <winsock2.h>
#include <windows.h>
#include <io.h>
#elif linux
#include <sys/io.h>
#include <sys/stat.h>
#endif

#include <iostream>
#include <ctime>
#include <fstream>
#include <thread>
#include <stdarg.h>
#include "SyncQueue.h"
#include "OSHelper.cpp"


using namespace std;

/**
 * 日志等级
 */
enum LogLevel {
    DEBUG, INFO
};


/**
 * 日志类
 */
class Logger {
private:
    thread *pLoggerThread;
    void startLoggerThreadLo();

    static LogLevel logLevel;

public:
    static mutex printLock;
    static SyncQueue<string> msgQueue;       // 消息队列
    static bool isWriteToFile;
    static bool isPrintInfo;
    static bool isAsyncLog;

    /**
     *
     *
     * @param isPrintInfo
     * @param isWriteToFile
     * @param isAsyncLog 异步日志
     */
    explicit Logger(bool isAsyncLog = true, bool isPrintInfo = true, bool isWriteToFile = true, bool isCleanOldLogFile = true):
            pLoggerThread(NULL)
    {
        Logger::isWriteToFile = isWriteToFile;
        Logger::isPrintInfo = isPrintInfo;
        Logger::isAsyncLog = isAsyncLog;

        if (isAsyncLog) {
            startLoggerThreadLo();
        }

        if (isCleanOldLogFile) {
            deleteLogFile();
        }
    }

    /**
     * 设定日志等级
     */
    static void setLogLevel(LogLevel logLevel) {
        Logger::logLevel = logLevel;
    }


    static void print(ostream *pOut, string msg);

    static string getLogFilePath();

    static string getCurrentTime();

    static void *t_write();

    static string getFormattedStrCore(const char *format, va_list arg);

    //static void log(ostream *pOut, const char *format, ...);

    static string getPrefix();

    static string getFormattedStr(const char *format, ...);

    static void log(string s);

    static bool isWriteThreadWorking();

    static void deleteLogFile();
};

/**
 * 静态变量初始化（只能在类外部）
 */
bool Logger::isWriteToFile = true;
bool Logger::isPrintInfo = true;
bool Logger::isAsyncLog = false;    // 默认同步打印
mutex Logger::printLock;
SyncQueue<string> Logger::msgQueue;
LogLevel Logger::logLevel = DEBUG;

/**
 * Info 打印
 */
#define info(...) {\
    string sPrefix = Logger::getPrefix(); \
    string info = Logger::getFormattedStr(__VA_ARGS__);\
    string s = Logger::getFormattedStr("%s%s", sPrefix.c_str(), info.c_str());\
    Logger::log(s);\
}

/**
 * TODO Debug 打印
 */
#define debug(...) {\
    string sPrefix = Logger::getPrefix(); \
    string info = Logger::getFormattedStr(__VA_ARGS__);\
    string s = Logger::getFormattedStr("%s%s", sPrefix.c_str(), info.c_str());\
    Logger::log(s);\
}

/**
 * Error 打印
 *
 * 主要用到了 __FILE__ 和 __LINE__ 这两个宏
 */
#define err(...) {\
    string sPrefix = Logger::getPrefix();\
    string lineInfo = Logger::getFormattedStr("(%s:%d)", __FILE__, __LINE__);\
    string errInfo = Logger::getFormattedStr(__VA_ARGS__);\
    string s = Logger::getFormattedStr("%s%s\n%s", sPrefix.c_str(), lineInfo.c_str(), errInfo.c_str());\
    Logger::log(s);\
}

/**
 * 启动写文件线程
 */
void Logger::startLoggerThreadLo() {
    pLoggerThread = new thread(t_write);
    pLoggerThread->detach();
}

/**
 * TODO 删除日志文件
 */
void Logger::deleteLogFile() {
    string logPath = getLogFilePath();
//    if () {
//
//    }
}

/**
 * 获取写日志文件线程是否还在运行
 */
bool Logger::isWriteThreadWorking() {

    return false;
}

/**
 * 线程函数，从消息队列中取消息，然后写入文件
 */
void *Logger::t_write() {
    info(" logger thread tid: %ld\n", getThreadID());
    while (1) {
        //
        // 取数据，若没有数据，阻塞等待
        //
        string msg = msgQueue.get(true);

        //
        // 打印到控制台
        //
        if (isPrintInfo) {
            cout << msg << flush;
        }

        //
        // 写入日志文件
        //
        if (isWriteToFile) {
            string filePath = getLogFilePath();
            // -----------------------------------------
            // 【易错点】要以二进制格式写入，不然 \r\n 会被写成 \r\r\n
            // 以二进制格式写入，\r\n 就原模原样写入
            // -----------------------------------------
            ofstream logFile(filePath, ios::app | ios::binary);
            if (logFile) {
                logFile << msg;
            } else {
                print(&cerr, "log file open failed:" + filePath);
            }
            logFile.close();
        }
    }
}


/**
 * 输出到控制台
 */
void Logger::print(ostream *pOut, string msg) {
    string sPrefix = getPrefix();
    *pOut << sPrefix << msg << std::flush;  // 刷新缓冲区
}

/**
 * 获取格式化后的字符串
 *
 * @param format 格式
 * @param ... 参数
 */
string Logger::getFormattedStr(const char *format, ...) {
    va_list arg;
    va_start(arg, format);
    string s = getFormattedStrCore(format, arg);
    va_end(arg);

    return s;
}

/**
 * 获取格式化后的字符串，代码中做了长度限制，格式化后的字符串最长 1000
 *
 * @param format 格式
 * @param arg 变参指针
 */
string Logger::getFormattedStrCore(const char *format, va_list arg) {
    int done;
    const int len = 1001; // 字符数组长度
    char msg[len];

    //
    // 格式化字符串
    //
    // --------------------------------------------------
    // 【易错点】要用 vsnprintf，不能用 snprintf
    // 带 v 的都是使用变量 va_list 传递变参
    // --------------------------------------------------
    // FIXME 如果参数个数对不上通配符个数，这里会出错，并且没有任何提示，不会抛出异常，程序直接崩溃
    done = vsnprintf(msg, len - 1, format, arg);

    //
    // 字符串长度超过 len-4 时，最后面省略号表示，-1 表示已超过数组长度
    //
    if (done == -1 || done > 1000) {
        msg[len - 5] = '.';
        msg[len - 4] = '.';
        msg[len - 3] = '.';
        msg[len - 2] = '\n';
        msg[len - 1] = '\0';
    }
    else {
        msg[done] = '\0';
    }

    string formattedStr = string(msg);
    return formattedStr;
}

/**
 * 打印日志并写入文件
 *
 * @param pOut 输出对象
 * @param s 字符串
 */
void Logger::log(string s) {
    if (isAsyncLog) {
        Logger::msgQueue.put(s);
    } else {
        lock_guard<mutex> lockGuard(printLock);
        //
        // 打印到控制台
        //
        if (isPrintInfo) {
            cout << s << flush;
        }

        //
        // 写入日志文件
        //
        if (isWriteToFile) {
            string filePath = getLogFilePath();
            // -----------------------------------------
            // 【易错点】要以二进制格式写入，不然 \r\n 会被写成 \r\r\n
            // 以二进制格式写入，\r\n 就原模原样写入
            // -----------------------------------------
            ofstream logFile(filePath, ios::app | ios::binary);
            if (logFile) {
                logFile << s;
            } else {
                print(&cerr, "log file open failed:" + filePath);
            }
            logFile.close();
        }
    }
}

/**
 * 打日志
 *
 * @param pOut 输出对象，cout / cerr
 * @param format 格式
 * @param ... 参数
 */
/*
void Log::log(ostream *pOut, const char *format, ...) {
   // 获取格式化后的字符串
   va_list arg;
   va_start(arg, format);
   string s = getFormattedStrCore(format, arg);
   va_end(arg);

   // 打印字符串
   string sPrefix = getPrefix();
   *pOut << sPrefix << s << std::flush;;

   // 写入文件
   if (isWriteToFile) {
       write(s);
   }
}
 */

/**
 * 获取日志前缀
 */
string Logger::getPrefix() {
    //
    // 时间
    //
    string sTime = "[" + getCurrentTime() + "]";

    //
    // 线程 id
    //
    char sTid[21];
    snprintf(sTid, 20, "[tid %ld]", getThreadID());

    string s = sTime + sTid;
    return s;
}

/**
 * 获取日志文件路径
 *
 * 日志文件命名方式：年_月_日.txt，如 2021_03_09.txt
 * 换言之，每天都会创建一个日志文件
 */
string Logger::getLogFilePath() {
    //
    // 获取当前日期
    //
    // -----------------------------------------
    //
    //【易错点】lt->tm_mon 从 0 开始，所以用的时候要加 1
    //
    // -----------------------------------------
    char fileName[100];
    time_t t;
    struct tm *lt;
    time(&t);
    lt = localtime(&t);
    sprintf(fileName, "%d_%02d_%02d.txt", lt->tm_year + 1900, lt->tm_mon + 1, lt->tm_mday);

    //
    // 创建日志目录
    //
    string logDir("./log");
#ifdef WIN32
    int nRet = CreateDirectory(logDir.c_str(), NULL);  // 创建目录（该函数不可递归，只能创建终极目录）
#else
    // 权限为0777，即拥有者权限为读、写、执行
    int nRet = mkdir(logDir.c_str(), 0777);
#endif

    //
    // 返回日志文件路径
    //
    string logPath = logDir + "/" + fileName;
    return logPath;
}

/**
 * 获取当前时间
 */
string Logger::getCurrentTime() {
    char s[100];
    time_t t;
    struct tm *lt;

    //
    // 获取当前时间，可精度到秒，并进行格式化
    //
    // -----------------------------------
    //
    // t = time(NULL) 也可以
    // localtime(&t) 表示转为本地时区
    //
    // -----------------------------------
    time(&t);
    lt = localtime(&t);
    string sTime = getFormattedStr("%d/%02d/%02d %02d:%02d:%02d", lt->tm_year + 1900, lt->tm_mon + 1, lt->tm_mday,
                    lt->tm_hour, lt->tm_min, lt->tm_sec);
    return sTime;
}




