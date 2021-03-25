#pragma once

#include "Buffer.hpp"
#include "Channel.hpp"
#include "NetUtil.hpp"

#include <functional>
#include <memory>

class Connection;

using ConnectionPtr = std::shared_ptr<Connection>;
using ConnCallback = std::function<void(ConnectionPtr)>;
using ReadCallback = std::function<void(ConnectionPtr, Buffer *)>;
using WriteCallback = std::function<void(ConnectionPtr)>;
using CloseCallback = std::function<void(ConnectionPtr)>;

class AutoContext {
public:
    ~AutoContext() { deleter_(); }
    template <class T>
    T &get() {
        if (!ctx_) {
            T *t = new T();
            deleter_ = [t] { delete t; };
            ctx_ = t;
        }
        return *reinterpret_cast<T *>(ctx_);
    }
    template <class T>
    T &get(const T &t) {
        if (!ctx_) {
            T *t = new T(t);
            deleter_ = [t] { delete t; };
            ctx_ = t;
        }
        return *reinterpret_cast<T *>(ctx_);
    }
    template <class T>
    T &get(T &&t) {
        if (!ctx_) {
            T *t = new T(std::move(t));
            deleter_ = [t] { delete t; };
            ctx_ = t;
        }
        return *reinterpret_cast<T *>(ctx_);
    }

private:
    void *ctx_ = nullptr;
    std::function<void()> deleter_ = [] {};
};

class Connection : public std::enable_shared_from_this<Connection> {
public:
    enum State { Closed, Connected };
    Connection(EventLoop *loop, int fd, EndPoint local, EndPoint peer);
    ~Connection();
    void shutdown();
    void close();
    void write(const char *data, size_t len);
    void onRead(const ReadCallback &cb) { readCallback_ = cb; }
    void onWrite(const WriteCallback &cb) { writeCallback_ = cb; }
    void onClose(const CloseCallback &cb) { closeCallback_ = cb; }
    void onRead(ReadCallback &&cb) { readCallback_ = std::move(cb); }
    void onWrite(WriteCallback &&cb) { writeCallback_ = std::move(cb); }
    void onClose(CloseCallback &&cb) { closeCallback_ = std::move(cb); }
    void enableRead(bool enable) { channel_->enableRead(enable); }
    EndPoint getLocal() { return local_; }
    EndPoint getPeer() { return peer_; }
    State getState() { return state_; }
    int getFd() { return fd_; }
    AutoContext &context() { return *ctx_; }

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
    ReadCallback readCallback_;
    WriteCallback writeCallback_;
    CloseCallback closeCallback_;
    std::unique_ptr<AutoContext> ctx_;
};
