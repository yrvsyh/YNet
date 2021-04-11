#pragma once

#include "ThreadsafeQueue.hpp"

#include <functional>
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
    bool running_;
    std::vector<std::thread> threads_;
    ThreadsafeQueue<std::function<void()>> tasks_;
};
