#pragma once

#include <thread>
#include <functional>
#include "HttpResponse.cpp"
#include "SyncQueue.cpp"
#include "Logger.cpp"

using namespace std;

/**
 * 对线程池封装成类
 */
template<class QueueElement>
class ThreadPool {
private:
    int poolSize;       // 线程池容量
    mutex mutexWorkerNumber;
    int currentWorkerNumber;
    SyncQueue<QueueElement> taskQueue;  // 任务队列
    function<void()> onTaskFinishedCallback;

public:

    /**
     * 线程池初始化
     */
    explicit ThreadPool(int poolSize = 0):
            poolSize(poolSize), currentWorkerNumber(0), onTaskFinishedCallback(NULL){
        if (poolSize == 0) {
            this->poolSize = getCPULogicCoresNumber() + 1;
        }
        info(" poolSize: %d\n", this->poolSize);
    }

    bool submitTask(QueueElement task);


    QueueElement getTask();


    void setOnTaskFinishedCallback(function<void()> onTaskFinishedCallback);

    static void *worker_main(ThreadPool *pThreadPool);

    void subWorkerNumber();

    void addWorkerNumber();

};






/**
 * 子线程函数
 */
template<class QueueElement>
void* ThreadPool<QueueElement>::worker_main(ThreadPool<QueueElement> *pThreadPool) {
    //
    // 取任务并执行
    //
    pThreadPool->addWorkerNumber();
    while (true) {
        try {
            QueueElement element = pThreadPool->getTask();
            SOCKET connSocket = element.first;
            long acceptedTime = element.second;
            info("[socket %s] socket %d wait time: %d ms\n", getSocketIPPort(connSocket).c_str(), connSocket,
                 getTimeDiff(acceptedTime));
            HttpResponse response(connSocket);
            response.handleRequest();
            if (pThreadPool->onTaskFinishedCallback != NULL) {
                pThreadPool->onTaskFinishedCallback();
            }
        } catch(exception &e) {
            info(" worker finished, info: %s\n", e.what());
            break;
        }
    }

    //
    // 退出线程
    //
    pThreadPool->subWorkerNumber();

    return NULL;
}

/**
 * 提交任务到队列
 *
 * @return: 提交成功或失败
 */
template<class QueueElement>
bool ThreadPool<QueueElement>::submitTask(QueueElement task) {
    if (taskQueue.put(task)) {
        info(" task size: %d\n", taskQueue.getSize());
        if (currentWorkerNumber < poolSize) {
            thread worker(ThreadPool::worker_main, this);
            worker.detach();
        }
        return true;
    }

    return false;
}


/**
 * 获取任务
 */
template<class QueueElement>
QueueElement ThreadPool<QueueElement>::getTask() {
    return taskQueue.get();
}



template<class QueueElement>
void ThreadPool<QueueElement>::setOnTaskFinishedCallback(function<void()> onTaskFinishedCallback) {
    this->onTaskFinishedCallback = onTaskFinishedCallback;
}


template<class QueueElement>
void ThreadPool<QueueElement>::addWorkerNumber() {
    lock_guard<mutex> lockGuard(mutexWorkerNumber);
    currentWorkerNumber++;
    info(" addWorkerNumber: %d\n", currentWorkerNumber);
}

template<class QueueElement>
void ThreadPool<QueueElement>::subWorkerNumber() {
    lock_guard<mutex> lockGuard(mutexWorkerNumber);
    currentWorkerNumber--;
    info(" subWorkerNumber: %d\n", currentWorkerNumber);
}










