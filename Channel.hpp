#pragma once

#include <cstdint>
#include <functional>
#include <spdlog/spdlog.h>
#include <sys/epoll.h>

class EventLoop;

class Channel {
public:
    using Callback = std::function<void()>;
    enum Status { kNew, kAdded, kDeleted };

public:
    Channel(EventLoop *Loop, int fd = -1);
    ~Channel();
    int fd() { return fd_; }
    Status status() { return status_; }
    void enableRead(bool enable) {
        enable ? events_ |= EPOLLIN : events_ &= ~EPOLLIN;
        update();
    }
    void enableWrite(bool enable) {
        enable ? events_ |= EPOLLOUT : events_ &= ~EPOLLOUT;
        update();
    }
    void setEvents(uint32_t events) {
        events_ = events;
        update();
    }
    void handlerEvent();
    void remove();
    void onRead(Callback &&cb) { readCb_ = std::move(cb); }
    void onWrite(Callback &&cb) { writeCb_ = std::move(cb); }
    void onError(Callback &&cb) { errorCb_ = std::move(cb); }
    void onClose(Callback &&cb) { closeCb_ = std::move(cb); }

private:
    friend class Epoll;
    void setStatus(Status status) { status_ = status; }
    void update();
    uint32_t events() { return events_; }
    void setRevents(uint32_t revents) { revents_ = revents; }

private:
    EventLoop *loop_;
    int fd_;
    Status status_;
    uint32_t events_;
    uint32_t revents_;
    bool eventHandling_;
    bool addedToLoop_;

    Callback readCb_ = [this] { spdlog::debug("default read callback fd = {}", fd_); };
    Callback writeCb_ = [this] { spdlog::debug("default write callback fd = {}", fd_); };
    Callback errorCb_ = [this] { spdlog::debug("default error callback fd = {}", fd_); };
    Callback closeCb_ = [this] { spdlog::debug("default close callback fd = {}", fd_); };
};
