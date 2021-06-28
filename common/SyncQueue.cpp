#pragma once

#include <list>
#include <mutex>
#include <condition_variable>
#include "CrossPlatform.h"

using namespace std;

template <class T>
class SyncQueue {
private:
    list<T> m_queue;
    mutex m_mutex;  // 同步锁
    condition_variable m_notEmpty;    // 条件变量

public:
    explicit SyncQueue() {

    }


    /**
     * 放入一条信息
     */
    bool put(const T x) {
        unique_lock<mutex> locker(m_mutex);
        m_queue.push_back(x);
        m_notEmpty.notify_one();
        return true;
    }

    /**
     * 获取一条信息，没有则阻塞
     */
    T get() {
        unique_lock<mutex> locker(m_mutex);
        while(m_queue.empty()) {
            m_notEmpty.wait(locker);    // 解锁 m_mutex，并等待被唤醒
        }
        T x = m_queue.front();
        m_queue.pop_front();
        return x;
    }

    /**
     * 队列是否为空
     */
    bool isEmpty() {
        return m_queue.empty();
    }
};