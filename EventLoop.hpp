#pragma once

#include "Channel.hpp"
#include "Epoll.hpp"

#include <memory>
#include <mutex>
#include <signal.h>
#include <sys/signalfd.h>
#include <thread>
#include <vector>

class EventLoop {
public:
    using signalCallback = std::function<void(int)>;

    EventLoop();
    ~EventLoop();
    void loop();
    void runInLoop(std::function<void()> func);
    void queueInLoop(std::function<void()> func);
    void addSignal(int signal);
    void doPendingTask();
    void updateChannel(Channel *channel) { epoll_.updateChannel(channel); };
    void removeChannel(Channel *channel) { epoll_.removeChannel(channel); };
    void onSignal(signalCallback cb) { signalCb_ = cb; }
    void weakup();
    void quit();

private:
    Epoll epoll_;
    bool quit_;
    bool isLooping_;
    std::vector<Channel *> activeChannels_;
    std::unique_ptr<Channel> weakupChannel_;
    std::unique_ptr<Channel> signalChannel_;
    sigset_t sigmask_;
    std::vector<std::function<void()>> pendingTask_;
    std::mutex mutex_;
    signalCallback signalCb_ = [](int) {};
};

class EventThread {
public:
    EventThread();
    EventLoop *getLoop() { return loop_; }
    void join() { thread_->join(); }

private:
    EventLoop *loop_;
    std::unique_ptr<std::thread> thread_;
};
