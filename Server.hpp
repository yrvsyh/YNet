#pragma once

#include "Channel.hpp"
#include "Connection.hpp"
#include "EventLoop.hpp"

#include <memory>
#include <unordered_set>
#include <vector>

class Server {
public:
    Server(EventLoop *loop, std::string ip, int port);
    ~Server();
    void start(int threadNum, int maxConn = 1024);
    void onRead(const ReadCallback &cb) { readCb_ = cb; }
    void onConn(const ConnCallback &cb) { connCb_ = cb; }
    void onClose(const CloseCallback &cb) { closeCb_ = cb; }
    void onRead(ReadCallback &&cb) { readCb_ = std::move(cb); }
    void onConn(ConnCallback &&cb) { connCb_ = std::move(cb); }
    void onClose(CloseCallback &&cb) { closeCb_ = std::move(cb); }

private:
    void newConn();
    void closeConn(ConnectionPtr conn);

private:
    EventLoop *loop_;
    EndPoint endpoint_;
    int listenFd_;
    std::unique_ptr<Channel> listenChannel_;
    std::unordered_set<ConnectionPtr> conns_;
    uint64_t maxConn_;
    std::vector<std::unique_ptr<EventThread>> workers_;
    int workerNums_;
    ReadCallback readCb_ = [](ConnectionPtr, Buffer *) {};
    ConnCallback connCb_ = [](ConnectionPtr) {};
    CloseCallback closeCb_ = [](ConnectionPtr) {};
};
