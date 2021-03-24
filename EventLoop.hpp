#pragma once

#include "Channel.hpp"
#include "Epoll.hpp"

#include <bits/types/time_t.h>
#include <map>
#include <memory>
#include <mutex>
#include <signal.h>
#include <sys/signalfd.h>
#include <sys/timerfd.h>
#include <thread>
#include <vector>

struct Timer {
    Timer() : its{0} {}
    Timer(time_t after) {
        timespec now;
        clock_gettime(CLOCK_MONOTONIC, &now);
        its.it_value.tv_sec = now.tv_sec + after;
        its.it_value.tv_nsec = now.tv_nsec;
        its.it_interval = {0};
    }
    Timer(time_t after, time_t every) {
        timespec now;
        clock_gettime(CLOCK_MONOTONIC, &now);
        its.it_value.tv_sec = now.tv_sec + after;
        its.it_value.tv_nsec = now.tv_nsec;
        its.it_interval.tv_sec = every;
        its.it_interval.tv_nsec = 0;
    }
    Timer(timespec ts) {
        its.it_value = ts;
        its.it_interval = {0};
    }
    Timer(const Timer &t) = default;
    bool operator<(const Timer &t) const {
        if (its.it_value.tv_sec < t.its.it_value.tv_sec) {
            return true;
        } else if (its.it_value.tv_nsec < t.its.it_value.tv_nsec) {
            return true;
        }
        return false;
    }

    itimerspec its;
};

class EventLoop {
public:
    using signalCallback = std::function<void(int)>;
    using Task = std::function<void()>;

    EventLoop();
    ~EventLoop();
    void loop();
    void runInLoop(Task &&func);
    void queueInLoop(Task &&func);
    void addTimer(Timer t, Task &&func);
    void addSignal(int signal);
    void updateChannel(Channel *channel) { epoll_.updateChannel(channel); };
    void removeChannel(Channel *channel) { epoll_.removeChannel(channel); };
    void onSignal(signalCallback &&cb) { signalCb_ = std::move(cb); }
    void weakup();
    void quit();

private:
    void doPendingTask();
    void onTimer();

private:
    Epoll epoll_;
    bool quit_;
    bool isLooping_;
    std::vector<Channel *> activeChannels_;
    std::unique_ptr<Channel> weakupChannel_;
    std::unique_ptr<Channel> signalChannel_;
    std::unique_ptr<Channel> timerChannel_;
    sigset_t sigmask_;
    std::vector<Task> pendingTask_;
    std::map<Timer, Task> timerMap_;
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
