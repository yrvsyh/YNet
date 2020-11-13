#pragma once

#include "Channel.hpp"
#include "NetUtil.hpp"

#include <functional>
#include <memory>

class Connection : public std::enable_shared_from_this<Connection> {
public:
    enum State { Closed, Connecting, Connected };
    Connection(EventLoop *loop, int fd, EndPoint local, EndPoint peer);
    void close();
    void onRead(std::function<void()> cb) { readCallback_ = cb; }
    void onWrite(std::function<void()> cb) { writeCallback_ = cb; }
    void onState(std::function<void()> cb) { stateCallback_ = cb; }

private:
    void doRead();
    void doWrite();

private:
    std::unique_ptr<Channel> channel_;
    State state_;
    int fd_;
    EndPoint local_;
    EndPoint peer_;
    std::string inbuf_;
    std::string outbuf_;
    std::function<void()> readCallback_;
    std::function<void()> writeCallback_;
    std::function<void()> stateCallback_;
};
