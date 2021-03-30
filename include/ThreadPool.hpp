#pragma once

#include <condition_variable>
#include <deque>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

class ThreadPool {
public:
    ThreadPool() : running_(false) {}
    ~ThreadPool() {
        if (running_) {
            stop();
        }
    }
    void start(int threadNum = 1);
    void stop();
    void run(const std::function<void()> &task);
    void run(std::function<void()> &&task);

private:
    std::function<void()> getTask();

private:
    std::mutex mutex_;
    std::condition_variable empty_;
    bool running_;
    std::vector<std::thread> threads_;
    std::deque<std::function<void()>> tasks_;
};
