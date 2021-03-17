#pragma once

#include "Channel.hpp"
#include "Connection.hpp"
#include "EventLoop.hpp"

#include <memory>
#include <set>
#include <vector>

class Server {
public:
    Server(EventLoop *loop, std::string ip, int port);
    ~Server();
    void start(int threadNum);
    void onRead(ReadCallback cb) { readCb_ = cb; }
    void onConn(ConnCallback cb) { connCb_ = cb; }
    void onClose(CloseCallback cb) { closeCb_ = cb; }

private:
    void newConn();
    void closeConn(ConnectionPtr conn);

private:
    EventLoop *loop_;
    EndPoint endpoint_;
    int listenFd_;
    std::unique_ptr<Channel> listenChannel_;
    std::set<ConnectionPtr> conns_;
    std::vector<std::unique_ptr<EventThread>> workers_;
    ReadCallback readCb_ = [](ConnectionPtr, Buffer *) {};
    ConnCallback connCb_ = [](ConnectionPtr) {};
    CloseCallback closeCb_ = [](ConnectionPtr) {};
};
