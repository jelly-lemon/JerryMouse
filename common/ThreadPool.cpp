// 本文件包含了 ThreadPool 类、传给子线程的参数结构体 ThreadArgs
#pragma once

#include <thread>
#include "HttpResponse.cpp"
#include "SyncQueue.cpp"
#include "Logger.cpp"

using namespace std;


/**
 * 对线程池封装成类
 */
class ThreadPool {
private:
    int poolSize;       // 线程池容量
    mutex m_mutex;
    int currentThreadNumber;
    SyncQueue<SOCKET> socketQueue;  // 任务队列


public:

    /**
     * 线程池初始化
     */
    explicit ThreadPool(int poolSize):
    poolSize(poolSize), currentThreadNumber(0){

    }

    static void *worker_main(void *args);

    bool submit(SOCKET connSocket);

    void createNewThread();

    SOCKET getTask();

    void onWorkerFinished();

    static void handleSocket(SOCKET connSocket);
};






/**
 * 子线程函数
 */
void *ThreadPool::worker_main(void *args) {
    Logger::log(" new worker\n");

    //
    // 取任务并执行
    //
    auto *p_threadPool = (ThreadPool *)args;
    while (true) {
        try {
            SOCKET connSocket = p_threadPool->getTask();
            ThreadPool::handleSocket(connSocket);
        } catch(exception &e) {
            err("handleSocket failed, Err:%s\n", e.what());
            break;
        }
    }

    //
    // 退出线程
    //
    p_threadPool->onWorkerFinished();

    return NULL;
}

/**
 * 提交任务到队列
 *
 * @return: 提交成功或失败
 */
bool ThreadPool::submit(SOCKET connSocket) {
    if (socketQueue.put(connSocket)) {
        lock_guard<mutex> guard(m_mutex);
        if (currentThreadNumber < poolSize) {
            createNewThread();
        }
        return true;
    }

    return false;
}

/**
 * 创建新线程
 */
void ThreadPool::createNewThread() {
    pthread_t worker;
    pthread_create(&worker, NULL, ThreadPool::worker_main, (void*)this);
    currentThreadNumber++;
}

/**
 * 获取任务
 */
SOCKET ThreadPool::getTask() {
    return socketQueue.get();
}

/**
 * 某个线程结束时，现有线程数量减 1
 */
void ThreadPool::onWorkerFinished() {
    lock_guard<std::mutex> guard(m_mutex);
    currentThreadNumber--;
}

/**
 * 处理连接 socket
 *
 * @param connSocket
 */
void ThreadPool::handleSocket(SOCKET connSocket) {
    HttpResponse response(connSocket);
    response.handleRequest();
}








