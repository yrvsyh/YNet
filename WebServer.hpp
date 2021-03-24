#pragma once

#include "Server.hpp"

#include <map>
#include <string>

struct Session {
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
    void start(int threadNum) { server_.start(threadNum); }
    void setPrefix(std::string prefix) { prefix_ = prefix; }
    void setHomePage(std::string homePage) { homePage_ = homePage; }

private:
    void onRequest(ConnectionPtr conn, Buffer *buf);
    void replyClient(ConnectionPtr conn);

private:
    EventLoop *loop_;
    Server server_;
    std::map<ConnectionPtr, Session> sessions_;
    std::string prefix_;
    std::string homePage_;
};
