#pragma once

#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>

template <typename T> class ThreadsafeQueue {
public:
    ThreadsafeQueue(){};
    void push(const T &t) {
        std::lock_guard<std::mutex> lk(mutex_);
        queue_.push(t);
        cond_.notify_one();
    }
    void push(T &&t) {
        std::lock_guard<std::mutex> lk(mutex_);
        queue_.push(std::move(t));
        cond_.notify_one();
    }
    void wait_and_pop(T &t) {
        std::unique_lock<std::mutex> lk(mutex_);
        cond_.wait(lk, [this] { return !queue_.empty(); });
        t = std::move(queue_.front());
        queue_.pop();
    }
    std::shared_ptr<T> wait_and_pop() {
        std::unique_lock<std::mutex> lk(mutex_);
        cond_.wait(lk, [this] { return !queue_.empty(); });
        auto ret = std::make_shared<T>(queue_.front());
        queue_.pop();
        return ret;
    }
    bool try_pop(T &t) {
        std::lock_guard<std::mutex> lk(mutex_);
        if (queue_.empty()) {
            return false;
        }
        t = std::move(queue_.front());
        queue_.pop();
        return true;
    }
    std::shared_ptr<T> try_pop() {
        std::lock_guard<std::mutex> lk(mutex_);
        if (queue_.empty()) {
            return std::shared_ptr<T>();
        }
        auto ret = std::make_shared<T>(queue_.front());
        queue_.pop();
        return ret;
    }
    bool empty() {
        std::lock_guard<std::mutex> lk(mutex_);
        return queue_.empty();
    }

private:
    std::queue<T> queue_;
    std::mutex mutex_;
    std::condition_variable cond_;
};
