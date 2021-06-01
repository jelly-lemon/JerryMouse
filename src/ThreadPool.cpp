// 本文件包含了 ThreadPool 类、传给子线程的参数结构体 ThreadArgs
#pragma once

#include <pthread.h>
#include <winsock2.h>
#include "HttpResponse.cpp"

using namespace std;


/**
 * 对线程池封装成类
 */
class ThreadPool {
private:
    int poolSize;               // 线程池大小
    int currentNumber;          // 现有线程数量
    pthread_rwlock_t rwlock;    // 读写锁

    static void *handle_connection_main(void *args);

public:
    /**
     * 线程池初始化
     *
     * @param poolSize 线程池大小
     */
    explicit ThreadPool(int poolSize) {
        this->poolSize = poolSize;
        this->currentNumber = 0;    // 现有量为 0
        pthread_rwlock_init(&this->rwlock, NULL);      // 初始化读写锁
    }

    void startThread(SOCKET connSocket);

    void addCurrentNumber();

    void subCurrentNumber();
};

/**
 * 对要传给子线程函数的内容封装成一个结构体
 */
struct ThreadArgs {
    SOCKET connSocket;          // socket 描述符
    ThreadPool *pThreadPool;    // 线程池对象的地址
};


/**
 * 获取一个可用的线程
 *
 * 如果暂时没有可用的线程，就一直等待
 */
void ThreadPool::startThread(SOCKET connSocket) {
    // 等待线程池空位
    while (1) {
        pthread_rwlock_rdlock(&rwlock); // 读加锁
        // 如果现有线程数量 >= 线程池容量，就睡眠 0.1s
        if (this->currentNumber >= this->poolSize) {
            pthread_rwlock_unlock(&rwlock); // 读解锁
            // Sleep(100);
        } else {
            // 【易错点】注意这里给线程传参，传的是地址，如果是局部变量的地址，这个函数一结束，局变就没了，
            // 子线程拿到这个地址再去取数据，就是有问题的。
            ThreadArgs *args = new ThreadArgs{connSocket, this};
            // 创建线程
            pthread_t t;
            pthread_create(&t, NULL, handle_connection_main, (void *) args);
            pthread_rwlock_unlock(&rwlock); // 读解锁

            // 线程现有量加 1
            addCurrentNumber();
            break;
        }
    }
}


/**
 * 线程池现有数量加 1
 */
void ThreadPool::addCurrentNumber() {
    pthread_rwlock_wrlock(&rwlock); // 写加锁
    currentNumber++;
    pthread_rwlock_unlock(&rwlock); // 写解锁
}

/**
 * 线程池现有数量减 1
 */
void ThreadPool::subCurrentNumber() {
    pthread_rwlock_wrlock(&rwlock); // 写加锁
    currentNumber--;
    pthread_rwlock_unlock(&rwlock); // 写解锁
}

/**
 * 处理连接需要开启的子线程函数
 */
void *ThreadPool::handle_connection_main(void *args) {
    // 解析参数
    ThreadArgs *pThreadArgs = (ThreadArgs *) args;
    SOCKET connSocket = pThreadArgs->connSocket;
    ThreadPool *pThreadPool = pThreadArgs->pThreadPool;
    delete pThreadArgs;

    // 对客户端请求进行响应
    HttpResponse response(connSocket);
    response.handleRequest();

    // 线程池现有线程数量减 1
    pThreadPool->subCurrentNumber();

    return NULL;
}







