#pragma once

#include <cstdint>
#include <functional>
#include <spdlog/spdlog.h>

class EventLoop;

class Channel {
public:
    enum Status { kNew, kAdded, kDeleted };

public:
    Channel(EventLoop *Loop, int fd = -1);
    ~Channel();
    int fd() { return fd_; }
    Status status() { return status_; }
    void setStatus(Status status) { status_ = status; }
    void update();
    void remove();
    uint32_t events() { return events_; }
    void setEvents(uint32_t events) {
        events_ = events;
        update();
    }
    void setRevents(uint32_t revents) { revents_ = revents; }
    void handlerEvent();
    void onRead(std::function<void()> cb) { readCallback_ = std::move(cb); }
    void onWrite(std::function<void()> cb) { writeCallback_ = std::move(cb); }
    void onError(std::function<void()> cb) { errorCallback_ = std::move(cb); }
    void onClose(std::function<void()> cb) { closeCallback_ = std::move(cb); }

private:
    EventLoop *loop_;
    int fd_;
    Status status_;
    uint32_t events_;
    uint32_t revents_;
    bool eventHandling_;
    bool addedToLoop_;

    std::function<void()> readCallback_ = [] { spdlog::debug("default read callback"); };
    std::function<void()> writeCallback_ = [] { spdlog::debug("default write callback"); };
    std::function<void()> errorCallback_ = [] { spdlog::debug("default error callback"); };
    std::function<void()> closeCallback_ = [] { spdlog::debug("default close callback"); };
};
