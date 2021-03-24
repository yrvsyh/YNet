#include "EventLoop.hpp"
#include "Channel.hpp"

#include <spdlog/spdlog.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/signal.h>
#include <sys/signalfd.h>
#include <unistd.h>

__thread EventLoop *pEventLoopInThisThread = nullptr;

constexpr int kEpollWaitTimeout = 5000;

EventLoop::EventLoop() : quit_(false), isLooping_(false) {
    assert(pEventLoopInThisThread == nullptr);
    pEventLoopInThisThread = this;

    int weakupFd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    assert(weakupFd > 0);
    spdlog::debug("weakupFd = {}", weakupFd);
    weakupChannel_.reset(new Channel(this, weakupFd));
    weakupChannel_->onRead([this] {
        spdlog::debug("weakuped");
        uint64_t one = 1;
        ::read(weakupChannel_->fd(), &one, sizeof(one));
    });
    weakupChannel_->setEvents(EPOLLIN);

    ::sigemptyset(&sigmask_);
    ::sigprocmask(SIG_BLOCK, &sigmask_, nullptr);
    int sigFd = ::signalfd(-1, &sigmask_, SFD_NONBLOCK | SFD_CLOEXEC);
    assert(sigFd > 0);
    spdlog::debug("sigFd = {}", sigFd);
    signalChannel_.reset(new Channel(this, sigFd));
    signalChannel_->onRead([this] {
        struct signalfd_siginfo siginfo;
        ::read(signalChannel_->fd(), &siginfo, sizeof(siginfo));
        spdlog::debug("recv signal {}", siginfo.ssi_signo);
        signalCb_(siginfo.ssi_signo);
    });
    signalChannel_->setEvents(EPOLLIN);
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

void EventLoop::runInLoop(std::function<void()> &&func) {
    if (pEventLoopInThisThread == this) {
        func();
    } else {
        queueInLoop(std::move(func));
    }
}

void EventLoop::queueInLoop(std::function<void()> &&func) {
    if (pEventLoopInThisThread != this) {
        weakup();
    }
    std::lock_guard<std::mutex> lock(mutex_);
    pendingTask_.push_back(std::move(func));
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
    spdlog::debug("weakup loop");
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
