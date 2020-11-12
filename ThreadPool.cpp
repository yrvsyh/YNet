#include "ThreadPool.hpp"

#include <signal.h>
#include <spdlog/spdlog.h>

void ThreadPool::start(int threadNum) {
    spdlog::debug("starting thread poll");
    running_ = true;
    while (threadNum--) {
        threads_.emplace_back(std::thread([this] {
            sigset_t mask;
            ::sigfillset(&mask);
            ::sigprocmask(SIG_BLOCK, &mask, nullptr);
            while (running_) {
                getTask()();
            }
        }));
    }
}

void ThreadPool::stop() {
    spdlog::debug("stoping thread poll");
    running_ = false;
    empty_.notify_all();
    for (auto &thread : threads_) {
        thread.join();
    }
    threads_.clear();
    spdlog::debug("thread poll stoped");
}

void ThreadPool::run(std::function<void()> task) {
    std::unique_lock<std::mutex> lock(mutex_);
    tasks_.emplace_back(task);
    empty_.notify_all();
}

std::function<void()> ThreadPool::getTask() {
    std::unique_lock<std::mutex> lock(mutex_);
    while (running_ && tasks_.empty()) {
        empty_.wait(lock);
    }
    std::function<void()> task = [] {};
    if (!tasks_.empty()) {
        spdlog::trace("get one task");
        task = tasks_.front();
        tasks_.pop_front();
    }
    return task;
}
