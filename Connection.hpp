#pragma once

#include "Buffer.hpp"
#include "Channel.hpp"
#include "NetUtil.hpp"

#include <functional>
#include <memory>

class Connection;

using ConnectionPtr = std::shared_ptr<Connection>;
using ReadCallback = std::function<void(ConnectionPtr, Buffer *)>;
using ConnCallback = std::function<void(ConnectionPtr)>;
using CloseCallback = std::function<void(ConnectionPtr)>;

class Connection : public std::enable_shared_from_this<Connection> {
public:
    enum State { Closed, Connected };
    Connection(EventLoop *loop, int fd, EndPoint local, EndPoint peer);
    ~Connection();
    void shutdown();
    void close();
    void write(const char *data, size_t len);
    void onRead(std::function<void(const ConnectionPtr, Buffer *)> cb) { readCallback_ = cb; }
    void onWrite(std::function<void(const ConnectionPtr)> cb) { writeCallback_ = cb; }
    void onClose(std::function<void(const ConnectionPtr)> cb) { closeCallback_ = cb; }
    void enableRead(bool enable) { channel_->enableRead(enable); }
    EndPoint getLocal() { return local_; }
    EndPoint getPeer() { return peer_; }
    State getState() { return state_; }
    int getFd() { return fd_; }

private:
    void doRead();
    void doWrite();
    void doClose();

private:
    EventLoop *loop_;
    std::unique_ptr<Channel> channel_;
    State state_;
    int fd_;
    EndPoint local_;
    EndPoint peer_;
    Buffer readbuf_;
    Buffer writebuf_;
    std::function<void(const ConnectionPtr conn, Buffer *buf)> readCallback_;
    std::function<void(const ConnectionPtr conn)> writeCallback_;
    std::function<void(const ConnectionPtr conn)> closeCallback_;
};
