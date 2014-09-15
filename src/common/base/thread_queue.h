#ifndef THREAD_QUEUE_H
#define THREAD_QUEUE_H

#include <deque>
#include <thread>
#include <condition_variable>
#include <mutex>
#include <assert.h>

// 线程安全的队列
namespace sails {
namespace common {

template<typename T, typename D = std::deque<T> >
class ThreadQueue {
public:
    ThreadQueue():_size(0) { _maxSize = 100000;}
public:
    typedef D queue_type;
    // 从头部获取数据, 没有数据则等待.
    // @param millsecond   阻塞等待时间(ms)
    //                     0 表示不阻塞  -1 永久等待
    bool pop_front(T& t, int millsecond = 0);

    // 通知等待在队列上面的线程都醒过来
    void notifyT();

    // 放数据到队列后端.
    void push_back(const T& t);

    // 放数据到队列后端
    void push_back(const queue_type &qt);
    
    // 放数据到队列前端. 
    void push_front(const T& t);

    // 放数据到队列前端. 
    void push_front(const queue_type &qt);

    // 等到有数据才交换.
    // millsecond  阻塞等待时间(ms)
    //             0 表示不阻塞 -1 为则永久等待
    // 有数据返回true, 无数据返回false
    bool swap(queue_type &q, int millisecond = 0);

    // 队列大小.
    size_t size();

    // 设置队列最大大小
    void setMaxSize(size_t maxSize);

    // 清空队列
    void clear();

    // 是否数据为空.
    bool empty();

    // 等待
    void wait(std::unique_lock<std::mutex>& locker);
    bool timedWait(int millisecond, std::unique_lock<std::mutex>& locker);

private:
    // 队列
    queue_type          _queue;

    size_t              _maxSize;
    // 队列大小
    size_t              _size;
    // 互斥量
    std::mutex          queue_mutex;
    std::condition_variable     notify;
    bool                isTerminate;
};



template<typename T, typename D> void ThreadQueue<T, D>::wait(std::unique_lock<std::mutex>& locker) {
    while((_queue.size() == 0)  && (!isTerminate)) {
	notify.wait(locker);
    }
}

template<typename T, typename D> bool ThreadQueue<T, D>:: timedWait(int millisecond, std::unique_lock<std::mutex>& locker) {
    notify.wait_for(locker, std::chrono::milliseconds(millisecond));
    if (_queue.size() > 0) {
	return true;
    }
    return false;
}


template<typename T, typename D> bool ThreadQueue<T, D>::pop_front(T& t, int millisecond)
{
    std::unique_lock<std::mutex> locker(queue_mutex);
    if (_queue.empty())
    {
        if(millisecond == 0)
        {
            return false;
        }
        if(millisecond == (size_t)-1)
        {
	    wait(locker);
        }
        else
        {
	    timedWait(millisecond, locker);
        }
    }

    if (_queue.empty())
    {
        return false;
    }

    t = _queue.front();
    _queue.pop_front();
    assert(_size > 0);
    --_size;


    return true;
}

template<typename T, typename D> void ThreadQueue<T, D>::notifyT()
{
    std::unique_lock<std::mutex> locker(queue_mutex);
    this->notify.notify_all();
}

template<typename T, typename D> void ThreadQueue<T, D>::push_back(const T& t)
{
    std::unique_lock<std::mutex> locker(queue_mutex);
    this->notify.notify_one();

    _queue.push_back(t);
    ++_size;
}

template<typename T, typename D> void ThreadQueue<T, D>::push_back(const queue_type &qt)
{
    std::unique_lock<std::mutex> locker(queue_mutex);

    typename queue_type::const_iterator it = qt.begin();
    typename queue_type::const_iterator itEnd = qt.end();
    while(it != itEnd)
    {
        _queue.push_back(*it);
        ++it;
        ++_size;
	this->notify.notify_one();
    }
}

template<typename T, typename D> void ThreadQueue<T, D>::push_front(const T& t)
{
    std::unique_lock<std::mutex> locker(queue_mutex);

    this->notify.notify_one();

    _queue.push_front(t);

    ++_size;
}

template<typename T, typename D> void ThreadQueue<T, D>::push_front(const queue_type &qt)
{
    std::unique_lock<std::mutex> locker(queue_mutex);

    typename queue_type::const_iterator it = qt.begin();
    typename queue_type::const_iterator itEnd = qt.end();
    while(it != itEnd)
    {
        _queue.push_front(*it);
        ++it;
        ++_size;

	this->notify.notify_one();
    }
}

template<typename T, typename D> bool ThreadQueue<T, D>::swap(queue_type &q, int millisecond)
{
    std::unique_lock<std::mutex> locker(queue_mutex);

    if (_queue.empty())
    {
        if(millisecond == 0)
        {
            return false;
        }
        if(millisecond == (size_t)-1)
        {
            wait(locker);
        }
        else
        {
            //超时了
            if(!timedWait(millisecond, locker))
            {
                return false;
            }
        }
    }

    if (_queue.empty())
    {
        return false;
    }

    q.swap(_queue);
    _size = q.size();

    return true;
}

template<typename T, typename D> size_t ThreadQueue<T, D>::size()
{
    std::unique_lock<std::mutex> locker(queue_mutex);
    //return _queue.size();
    return _size;
}


template<typename T, typename D> void ThreadQueue<T, D>::setMaxSize(size_t maxSize) {
    _maxSize = maxSize;
}

template<typename T, typename D> void ThreadQueue<T, D>::clear()
{
    std::unique_lock<std::mutex> locker(queue_mutex);
    _queue.clear();
    _size = 0;
}

template<typename T, typename D> bool ThreadQueue<T, D>::empty() 
{
    std::unique_lock<std::mutex> locker(queue_mutex);
    return _queue.empty();
}




} // namespace common
} // namespace sails

#endif /* THREAD_QUEUE_H */
