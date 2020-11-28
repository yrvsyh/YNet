#pragma once

#include "Buffer.hpp"
#include "Channel.hpp"
#include "NetUtil.hpp"

#include <functional>
#include <memory>

class Connection : public std::enable_shared_from_this<Connection> {
public:
    using Ptr = std::shared_ptr<Connection>;
    enum State { Closed, Connected };
    Connection(EventLoop *loop, int fd, EndPoint local, EndPoint peer);
    ~Connection();
    void shutdown();
    void close();
    void write(const char *data, size_t len);
    void onRead(std::function<void(const Ptr, Buffer *)> cb) { readCallback_ = cb; }
    void onWrite(std::function<void(const Ptr)> cb) { writeCallback_ = cb; }
    void onClose(std::function<void(const Ptr)> cb) { closeCallback_ = cb; }
    void enableRead(bool enable) { channel_->enableRead(enable); }
    EndPoint getLocal() { return local_; }
    EndPoint getPeer() { return peer_; }
    State getState() { return state_; }
    int getFd() { return fd_; }

private:
    void doRead();
    void doWrite();

private:
    EventLoop *loop_;
    std::unique_ptr<Channel> channel_;
    State state_;
    int fd_;
    EndPoint local_;
    EndPoint peer_;
    Buffer readbuf_;
    Buffer writebuf_;
    std::function<void(const Ptr conn, Buffer *buf)> readCallback_;
    std::function<void(const Ptr conn)> writeCallback_;
    std::function<void(const Ptr conn)> closeCallback_;
};
