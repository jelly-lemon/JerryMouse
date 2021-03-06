#pragma once

#include <list>
#include <mutex>
#include <condition_variable>
#include "CrossPlatform.h"

using namespace std;

template <class QueueElement>
class SyncQueue {
private:
    list<QueueElement> m_queue;
    mutex m_mutex;  // 同步锁
    condition_variable m_notEmpty;    // 条件变量

public:
    explicit SyncQueue() {

    }


    /**
     * 放入一条信息
     */
    bool put(const QueueElement x) {
        unique_lock<mutex> locker(m_mutex);
        m_queue.push_back(x);
        m_notEmpty.notify_one();
        return true;
    }

    /**
     * 获取一条数据
     *
     * @param isBocking: 是否阻塞等待
     */
    QueueElement get(bool isBocking = false) {
        unique_lock<mutex> locker(m_mutex);
        if (isBocking) {
            while(m_queue.empty()) {
                m_notEmpty.wait(locker);    // 解锁 mutexWorkerNumber，并等待被唤醒
            }
        } else {
            if (m_queue.empty()) {
                throw runtime_error("Queue is empty");
            }
        }

        QueueElement x = m_queue.front();
        m_queue.pop_front();

        return x;
    }


    /**
     * 队列是否为空
     */
    bool isEmpty() {
        unique_lock<mutex> locker(m_mutex);
        return m_queue.empty();
    }

    int getSize() {
        unique_lock<mutex> locker(m_mutex);
        return m_queue.size();
    }
};