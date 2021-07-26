#pragma once

#include <thread>
#include <functional>
#include "http/HttpResponse.h"
#include "SyncQueue.h"
#include "Logger.h"

using namespace std;

/**
 * 对线程池封装成类
 */
class ThreadPool {
private:
    int poolSize;                           // 线程池容量
    int currentWorkerNumber;                // 当前 worker 数量
    mutex mutexWorkerNumber;                // currentWorkerNumber 互斥量
    SyncQueue<function<void()>> taskQueue;  // 任务队列
    bool isBlockingWaitTask;                // 是否阻塞等待

public:
    /**
     * 线程池初始化
     */
    explicit ThreadPool(int poolSize = 0, bool isBlockingWaitTask = true):
    poolSize(poolSize),currentWorkerNumber(0), isBlockingWaitTask(isBlockingWaitTask){
        if (poolSize == 0) {
            this->poolSize = 2*getCPULogicCoresNumber() + 1;
        }
        info(" new ThreadPool, poolSize: %d\n", this->poolSize);
        startWorker();
    }

    /**
     * 创建工作线程
     */
    void startWorker() {
        int n = poolSize - currentWorkerNumber;
        for (int i = 0; i < n; i++) {
            thread worker(ThreadPool::worker_main, this);
            worker.detach();
        }
    }

    /**
     * 子线程函数
     */
    static void* worker_main(ThreadPool *pThreadPool) {
        //
        // 取任务并执行
        //
        info(" new worker: %d\n", getThreadID());
        pThreadPool->addWorkerNumber();
        while (true) {
            try {
                function<void()> task = pThreadPool->getTask(pThreadPool->isBlockingWaitTask);
                task();
            } catch(exception &e) {
                info(" task error, Err: %s\n", e.what());
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
    bool submitTask(function<void()> task) {
        if (taskQueue.put(task)) {
            info(" submitTask: %d\n", taskQueue.getSize());
            return true;
        } else {
            info(" submitTask failed, task size: %d, it's full\n", taskQueue.getSize());
        }

        return false;
    }

    function<void()> getTask(bool isBlocking = false) {
        function<void()> task = taskQueue.get(isBlocking);
        info(" getTask: %d\n", taskQueue.getSize());

        return task;
    }


    void addWorkerNumber() {
        lock_guard<mutex> lockGuard(mutexWorkerNumber);
        currentWorkerNumber++;
        info(" addWorkerNumber: %d\n", currentWorkerNumber);
    }

    void subWorkerNumber() {
        lock_guard<mutex> lockGuard(mutexWorkerNumber);
        currentWorkerNumber--;
        info(" subWorkerNumber: %d\n", currentWorkerNumber);
    }
};

















