#ifndef THREAD_SAFE_QUEUE_HPP
#define THREAD_SAFE_QUEUE_HPP

#include <queue>
#include <mutex>

template<class T, class Container = std::deque<T>>
class ThreadSafeQueue
{
    using value_type      = typename Container::value_type;
    using reference       = typename Container::reference;
    using const_reference = typename Container::const_reference;
    using size_type       = typename Container::size_type;

public:
    bool Empty() const
    {
        std::lock_guard<std::mutex> lock(_mutex);
        return _queue.empty();
    }

    size_type Size() const
    {
        std::lock_guard<std::mutex> lock(_mutex);
        return _queue.size();
    }

    void Push(value_type&& val)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _queue.push(val);
    }

    void Push(const value_type& val)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _queue.push(val);
    }

    void Pop()
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _queue.pop();
    }

    T GetTop()
    {
        std::lock_guard<std::mutex> lock(_mutex);
        T item{std::move(_queue.front())};
        _queue.pop();
        return item;
    }

    const_reference Front() const
    {
        std::lock_guard<std::mutex> lock(_mutex);
        return _queue.front();
    }

    const_reference Back() const
    {
        std::lock_guard<std::mutex> lock(_mutex);
        return _queue.back();
    }

private:
    std::queue<T, Container> _queue;
    mutable std::mutex       _mutex;
};

#endif // !THREAD_SAFE_QUEUE_HPP
