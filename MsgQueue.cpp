#include <list>
#include <mutex>
#include <condition_variable>

using namespace std;

class MsgQueue {
private:
    list<string> m_queue;
    mutex m_mutex;  // 同步锁
    condition_variable m_notEmpty;    // 条件变量

public:
    explicit MsgQueue() {

    }


    /**
     * 放入一条信息
     */
    bool put(const string x) {
        unique_lock<mutex> locker(m_mutex);
        m_queue.push_back(x);
        m_notEmpty.notify_one();
        return true;
    }

    /**
     * 获取一条信息，没有则阻塞
     */
    string get() {
        unique_lock<mutex> locker(m_mutex);
        while(m_queue.empty()) {
            m_notEmpty.wait(locker);    // 解锁 m_mutex，并等待被唤醒
        }
        string x = m_queue.front();
        m_queue.pop_front();
        return x;
    }

    bool isEmpty() {
        return m_queue.empty();
    }
};