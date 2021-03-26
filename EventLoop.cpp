#include "EventLoop.hpp"
#include "Channel.hpp"

#include <algorithm>
#include <cstring>
#include <ctime>
#include <errno.h>
#include <iterator>
#include <spdlog/spdlog.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/signal.h>
#include <sys/signalfd.h>
#include <sys/timerfd.h>
#include <unistd.h>

__thread EventLoop *pEventLoopInThisThread = nullptr;

constexpr int kEpollWaitTimeout = 5000;

EventLoop::EventLoop() : quit_(false), isLooping_(false) {
    assert(pEventLoopInThisThread == nullptr);
    pEventLoopInThisThread = this;

    int weakupFd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (weakupFd < 0) {
        spdlog::critical("create weakupFd error");
        exit(errno);
    }
    SPDLOG_DEBUG("weakupFd = {}", weakupFd);
    weakupChannel_.reset(new Channel(this, weakupFd));
    weakupChannel_->onRead([this] {
        SPDLOG_DEBUG("weakuped");
        uint64_t one = 1;
        ::read(weakupChannel_->fd(), &one, sizeof(one));
    });
    weakupChannel_->enableRead(true);

    ::sigemptyset(&sigmask_);
    ::sigprocmask(SIG_BLOCK, &sigmask_, nullptr);
    int sigFd = ::signalfd(-1, &sigmask_, SFD_NONBLOCK | SFD_CLOEXEC);
    if (sigFd < 0) {
        spdlog::critical("create sigFd error");
        exit(errno);
    }
    SPDLOG_DEBUG("sigFd = {}", sigFd);
    signalChannel_.reset(new Channel(this, sigFd));
    signalChannel_->onRead([this] {
        struct signalfd_siginfo siginfo;
        ::read(signalChannel_->fd(), &siginfo, sizeof(siginfo));
        SPDLOG_DEBUG("recv signal {}", siginfo.ssi_signo);
        signalCb_(siginfo.ssi_signo);
    });
    signalChannel_->enableRead(true);

    int timerFd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    if (timerFd < 0) {
        spdlog::critical("create timerFd error");
        exit(errno);
    }
    SPDLOG_DEBUG("timerFd = {}", timerFd);
    timerChannel_.reset(new Channel(this, timerFd));
    timerChannel_->onRead([this] { onTimer(); });
    timerChannel_->enableRead(true);
}

EventLoop::~EventLoop() {
    assert(pEventLoopInThisThread == this);
    pEventLoopInThisThread = nullptr;
    weakupChannel_->setEvents(0);
    weakupChannel_->remove();
    ::close(weakupChannel_->fd());
    signalChannel_->setEvents(0);
    signalChannel_->remove();
    ::close(signalChannel_->fd());
}

void EventLoop::loop() {
    isLooping_ = true;
    while (!quit_) {
        activeChannels_.clear();
        epoll_.wait(activeChannels_, kEpollWaitTimeout);
        for (const auto channel : activeChannels_) {
            channel->handlerEvent();
        }
        doPendingTask();
    }
    isLooping_ = false;
}

void EventLoop::runInLoop(Task &&func) {
    if (pEventLoopInThisThread == this) {
        func();
    } else {
        queueInLoop(std::move(func));
    }
}

void EventLoop::queueInLoop(Task &&func) {
    if (pEventLoopInThisThread != this) {
        weakup();
    }
    std::lock_guard<std::mutex> lock(mutex_);
    pendingTask_.push_back(std::move(func));
}

void EventLoop::addTimer(Timer t, Task &&func) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto last = timerMap_.begin();
    if (t < last->first) {
        int ret = timerfd_settime(timerChannel_->fd(), TFD_TIMER_ABSTIME, &t.its, nullptr);
        if (ret < 0) {
            spdlog::error("set timer error");
        }
    }
    timerMap_.emplace(t, func);
}

static timespec operator+(const timespec &t1, const timespec &t2) {
    timespec t;
    if (1000000000 - t1.tv_nsec <= t2.tv_nsec) {
        t.tv_sec = t1.tv_sec + t2.tv_sec + 1;
        t.tv_nsec = t2.tv_nsec - (1000000000 - t1.tv_nsec);
    } else {
        t.tv_sec = t1.tv_sec + t2.tv_sec;
        t.tv_nsec = t1.tv_nsec + t2.tv_nsec;
    }
    return t;
}

Timer::Timer(time_t after, long nsec, time_t every, long everyn) {
    timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    timespec tmp;
    tmp.tv_sec = after;
    tmp.tv_nsec = nsec;
    its.it_value = now + tmp;
    its.it_interval.tv_sec = every;
    its.it_interval.tv_nsec = everyn;
}

void EventLoop::onTimer() {
    uint64_t exp = 0;
    read(timerChannel_->fd(), &exp, sizeof(exp));
    timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    auto end = timerMap_.lower_bound(now);
    std::vector<std::pair<Timer, Task>> tasks;
    std::copy(timerMap_.begin(), end, std::back_inserter(tasks));
    timerMap_.erase(timerMap_.begin(), end);
    for (const auto &task : tasks) {
        auto t = task.first;
        if (t.its.it_interval.tv_sec != 0 || t.its.it_interval.tv_nsec != 0) {
            t.its.it_value = t.its.it_value + t.its.it_interval;
            timerMap_.emplace(t, task.second);
        }
    }
    for (const auto &task : tasks) {
        task.second();
    }
    itimerspec its = {{0, 0}, {0, 0}};
    if (!timerMap_.empty()) {
        its = timerMap_.begin()->first.its;
    }
    int ret = timerfd_settime(timerChannel_->fd(), TFD_TIMER_ABSTIME, &its, nullptr);
    if (ret < 0) {
        spdlog::error("set timer error: {}", strerror(errno));
        timerMap_.clear();
        itimerspec its = {{0, 0}, {0, 0}};
        timerfd_settime(timerChannel_->fd(), TFD_TIMER_ABSTIME, &its, nullptr);
    }
}

void EventLoop::addSignal(int signal) {
    sigaddset(&sigmask_, signal);
    sigprocmask(SIG_BLOCK, &sigmask_, nullptr);
    signalfd(signalChannel_->fd(), &sigmask_, SFD_NONBLOCK | SFD_CLOEXEC);
}

void EventLoop::doPendingTask() {
    std::vector<std::function<void()>> tasks;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        tasks.swap(pendingTask_);
    }
    for (const auto &task : tasks) {
        task();
    }
}

void EventLoop::weakup() {
    SPDLOG_DEBUG("weakup loop");
    uint64_t one = 1;
    ::write(weakupChannel_->fd(), &one, sizeof(one));
}

void EventLoop::quit() {
    quit_ = true;
    if (pEventLoopInThisThread != this) {
        weakup();
    }
}

EventThread::EventThread() {
    thread_.reset(new std::thread([this] {
        sigset_t mask;
        ::sigfillset(&mask);
        ::sigprocmask(SIG_BLOCK, &mask, nullptr);
        loop_ = new EventLoop();
        loop_->loop();
        delete loop_;
    }));
}
