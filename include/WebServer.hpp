#pragma once

#include "Server.hpp"

#include <string>

struct Session : AutoContext::Context {
    enum READ_STATE { REQUEST, HEADERS, BODY } state;
    struct Request {
        std::string method;
        std::string url;
        std::unordered_map<std::string, std::string> headers;
        size_t body_len;
        std::string body;
    } request;
    struct Response {
        std::string msg;
        std::string headers;
        std::string body;
    } response;
};

class WebServer {
public:
    WebServer(EventLoop *loop, std::string ip, int port);
    void start(int threadNum, int maxConn = 1024) { server_.start(threadNum, maxConn); }
    void setPrefix(std::string prefix) { prefix_ = prefix; }
    void setHomePage(std::string homePage) { homePage_ = homePage; }

private:
    void onRequest(ConnectionPtr conn, Buffer *buf);
    void replyClient(ConnectionPtr conn);

private:
    EventLoop *loop_;
    Server server_;
    std::string prefix_;
    std::string homePage_;
};
