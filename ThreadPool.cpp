#include <pthread.h>
#include <winsock2.h>
#include <windows.h>
#include <iostream>
#include "HttpResponse.cpp"

using namespace std;


/**
 * 线程池
 */
class ThreadPool {
private:
    int poolSize;  // 线程池大小
    int currentNumber;    // 现有线程数量
    pthread_rwlock_t rwlock;    // 读写锁
public:
    ThreadPool(int poolSize = 30);
    void startThread(void*(*t_func)(void *), SOCKET &connSocket);
    void addCurrentNumber();
    void subCurrentNumber();
};

struct ThreadArgs {
    SOCKET connSocket;
    ThreadPool *pThreadPool;
};



/**
 * 获取一个可用的线程
 *
 * 如果暂时没有可用的线程，就一直等待
 */
void ThreadPool::startThread(void*(*t_func)(void *), SOCKET &connSocket) {
    // 等待线程池空位
    while (1) {
        pthread_rwlock_rdlock(&rwlock); // 读加锁
        if (this->currentNumber >= this->poolSize) {
            pthread_rwlock_unlock(&rwlock);             // 读解锁
            Sleep(100);
        } else {
            ThreadArgs args = {connSocket, this};
            // 创建线程
            pthread_t t;
            pthread_create(&t, NULL, t_func, (void*)&args);
            pthread_rwlock_unlock(&rwlock); // 读解锁

            // 线程现有量加 1
            addCurrentNumber();
            break;
        }
    }
}




ThreadPool::ThreadPool(int poolSize) {
    this->poolSize = poolSize;
    this->currentNumber = 0;
    // 初始化读写锁
    pthread_rwlock_init(&this->rwlock, NULL);
}

void ThreadPool::addCurrentNumber() {
    pthread_rwlock_wrlock(&rwlock); // 写加锁
    currentNumber++;
    pthread_rwlock_unlock(&rwlock); // 写解锁
}

void ThreadPool::subCurrentNumber() {
    pthread_rwlock_wrlock(&rwlock); // 写加锁
    currentNumber--;
    pthread_rwlock_unlock(&rwlock); // 写解锁
}









