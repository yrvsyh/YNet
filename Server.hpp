#pragma once

#include "Channel.hpp"
#include "Connection.hpp"
#include "EventLoop.hpp"

#include <memory>
#include <thread>
#include <unordered_set>
#include <vector>

class Server {
public:
    Server(EventLoop *loop, std::string ip, int port);
    ~Server();
    void start(int threadNum);
    void onMsg(std::function<void(Connection::Ptr, Buffer *)> cb) { msgCallback_ = cb; }
    void onConn(std::function<void(Connection::Ptr)> cb) { newConnCallback_ = cb; }
    void onClose(std::function<void(Connection::Ptr)> cb) { closeCallback_ = cb; }

private:
    void newConn();
    void closeConn(Connection::Ptr conn);

private:
    EventLoop *loop_;
    EndPoint endpoint_;
    int listenFd_;
    std::unique_ptr<Channel> listenChannel_;
    std::unordered_set<Connection::Ptr> conns_;
    std::vector<std::unique_ptr<EventThread>> workers_;
    std::function<void(Connection::Ptr, Buffer *)> msgCallback_ = [](Connection::Ptr, Buffer *) {};
    std::function<void(Connection::Ptr)> newConnCallback_ = [](Connection::Ptr) {};
    std::function<void(Connection::Ptr)> closeCallback_ = [](Connection::Ptr) {};
};
