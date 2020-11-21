#pragma once

#include <cstdint>
#include <functional>
#include <spdlog/spdlog.h>
#include <sys/epoll.h>

class EventLoop;

class Channel {
public:
    enum Status { kNew, kAdded, kDeleted };

public:
    Channel(EventLoop *Loop, int fd = -1);
    ~Channel();
    int fd() { return fd_; }
    Status status() { return status_; }
    void setEvents(uint32_t events) {
        events_ = events;
        update();
    }
    void handlerEvent();
    void remove();
    void onRead(std::function<void()> cb) { readCallback_ = std::move(cb); }
    void onWrite(std::function<void()> cb) { writeCallback_ = std::move(cb); }
    void onError(std::function<void()> cb) { errorCallback_ = std::move(cb); }
    void onClose(std::function<void()> cb) { closeCallback_ = std::move(cb); }

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

    std::function<void()> readCallback_ = [&] { spdlog::debug("default read callback fd = {}", fd_); };
    std::function<void()> writeCallback_ = [&] { spdlog::debug("default write callback fd = {}", fd_); };
    std::function<void()> errorCallback_ = [&] { spdlog::debug("default error callback fd = {}", fd_); };
    std::function<void()> closeCallback_ = [&] { spdlog::debug("default close callback fd = {}", fd_); };
};
