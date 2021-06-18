/*
 * 阻塞队列
 *
 *
与条件变量搭配使用的「锁」，必须是 unique_lock，不能用 lock_guard。这个前面文章中已有说明。

等待前先加锁。等待时，如果条件不满足，wait 会原子性地解锁并把线程挂起。

条件变量被通知后，挂起的线程就被唤醒，但是唤醒也有可能是假唤醒，或者是因为超时等异常情况，
 所以被唤醒的线程仍要检查条件是否满足，所以 wait 是放在条件循环里面。
 cv.wait(lock, [] { return ready; }); 相当于：while (!ready) { cv.wait(lock); }。
 使用 lock_guard 对象来管理互斥锁，对象创建成功就表示成功获得锁，超出作用域就释放锁。
 主要区别在于unique_lock锁机制更加灵活，可以再需要的时候进行lock或者unlock调用，不非得是析构或者构造时。
 */
#include <list>
#include <mutex>
#include <winsock2.h>
using namespace std;

class TaskQueue {
private:
    list<SOCKET> m_queue;
    mutex m_mutex;
    int m_maxSize;


    bool isFull() {
        return m_queue.size() == m_maxSize;
    }

    bool isEmpty() {
        return m_queue.empty();
    }

public:
    explicit TaskQueue(int maxSize) : m_maxSize(maxSize) {

    }


    bool put(const SOCKET &x) {
        lock_guard<mutex> guard(m_mutex);
        if (isFull()) {
            return false;
        } else {
            m_queue.push_back(x);
            return true;
        }
    }

    SOCKET take() {
        lock_guard<mutex> guard(m_mutex);
        if (isEmpty()) {
            throw runtime_error("TaskQueue is empty");
        } else {
            SOCKET x = m_queue.front();
            m_queue.pop_front();
            return x;
        }
    }
};



