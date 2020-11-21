#pragma once

#include "Channel.hpp"
#include "Connection.hpp"
#include "EventLoop.hpp"

#include <map>
#include <memory>
#include <thread>
#include <vector>

class Server {
public:
    Server(EventLoop *loop, std::string ip, int port);
    void start(int threadNum);
    void onMsg(std::function<void(Connection::Ptr, Buffer *)> cb) { msgCallback_ = cb; }
    void onConn(std::function<void(Connection::Ptr)> cb) { newConnCallback_ = cb; }
    void onClose(std::function<void(Connection::Ptr)> cb) { closeCallback_ = cb; }

private:
    void newConn();
    void stateChanged(Connection::Ptr conn);

private:
    EventLoop *loop_;
    EndPoint endpoint_;
    int listenFd_;
    std::unique_ptr<Channel> listenChannel_;
    std::map<std::string, Connection::Ptr> conns_;
    std::vector<EventThread> wokers_;
    std::function<void(Connection::Ptr, Buffer *)> msgCallback_ = [](Connection::Ptr, Buffer *) {};
    std::function<void(Connection::Ptr)> newConnCallback_ = [](Connection::Ptr) {};
    std::function<void(Connection::Ptr)> closeCallback_ = [](Connection::Ptr) {};
};
