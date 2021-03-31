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
    int poolSize = 30;  // 线程池大小
    int currentSize = 0;
public:
    void startThread(SOCKET &connSocket);
    static void* handleConnection(void *args);

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
void ThreadPool::startThread(SOCKET &connSocket) {
    // 等待空位
    while (this->currentSize >= this->poolSize) {
        cout << this->currentSize << " ";
        Sleep(100);
    }
    cout << endl;

    // 创建线程
    ThreadArgs args = {connSocket, this};
    pthread_t t;
    pthread_create(&t, NULL, handleConnection, (void*)&args);

    // 线程现有量加 1
    this->currentSize++;
    cout << "new Thread, currentSize=" << this->currentSize << endl;
}

void *ThreadPool::handleConnection(void *args) {
    // 获取参数
    ThreadArgs threadArgs = *(ThreadArgs*)args;
    SOCKET connSocket = threadArgs.connSocket;
    ThreadPool *_this = threadArgs.pThreadPool;

    //cout << "in Thread, currentSize=" << _this->currentSize << endl;
    Sleep(2000);


    // 线程现有量减 1
    _this->currentSize--;

    return nullptr;
}







