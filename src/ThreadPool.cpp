#include "ThreadPool.hpp"

#include <signal.h>
#include <spdlog/spdlog.h>

void ThreadPool::start(int threadNum) {
    SPDLOG_DEBUG("starting thread poll");
    running_ = true;
    while (threadNum--) {
        threads_.emplace_back([this] {
            sigset_t mask;
            ::sigfillset(&mask);
            ::sigprocmask(SIG_BLOCK, &mask, nullptr);
            while (running_) {
                getTask()();
            }
        });
    }
}

void ThreadPool::stop() {
    SPDLOG_DEBUG("stoping thread poll");
    running_ = false;
    tasks_.push([] {});
    for (auto &thread : threads_) {
        thread.join();
    }
    threads_.clear();
    SPDLOG_DEBUG("thread poll stoped");
}

void ThreadPool::run(const std::function<void()> &task) { tasks_.push(task); }

void ThreadPool::run(std::function<void()> &&task) { tasks_.push(std::move(task)); }

std::function<void()> ThreadPool::getTask() {
    std::function<void()> task;
    tasks_.wait_and_pop(task);
    SPDLOG_TRACE("get one task");
    return task;
}
