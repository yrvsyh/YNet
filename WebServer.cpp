#include "WebServer.hpp"

#include <algorithm>
#include <spdlog/spdlog.h>
#include <sstream>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <unistd.h>

WebServer::WebServer(EventLoop *loop, std::string ip, int port)
    : loop_(loop), server_(loop_, ip, port), prefix_("../") {
    server_.onConn([this](Connection::Ptr conn) {
        std::lock_guard<std::mutex> lock(mutex_);
        sessions_.insert({conn.get(), Session()});
    });
    server_.onMsg([this](Connection::Ptr conn, Buffer *buf) { onRequest(conn, buf); });
    server_.onClose([this](Connection::Ptr conn) {
        std::lock_guard<std::mutex> lock(mutex_);
        sessions_.erase(conn.get());
    });
}

void WebServer::onRequest(Connection::Ptr conn, Buffer *buf) {
    auto &session = sessions_[conn.get()];
    bool haveMoreline = true;
    while (haveMoreline) {
        switch (session.state) {
        case Session::REQUEST: {
            Session::Request req;
            std::swap(session.request, req);
            auto &request = session.request;
            bool ok = false;
            auto line = buf->readUntil("\r\n", ok);
            if (ok) {
                std::stringstream sstream(line);
                sstream << line;
                sstream >> request.method >> request.url;
                spdlog::info("{} {} {}", conn->getPeer().toString(), request.method, request.url);
                session.state = Session::HEADERS;
                break;
            }
            haveMoreline = false;
            break;
        }
        case Session::HEADERS: {
            bool ok = false;
            auto line = buf->readUntil("\r\n", ok);
            if (ok) {
                auto &request = session.request;
                if (line.size() == 0) {
                    for (auto &header : request.headers) {
                        spdlog::trace("{}: {}", header.first, header.second);
                    }
                    auto iter = request.headers.find("Content-Length");
                    if (iter != request.headers.end()) {
                        request.body_len = atoi(iter->second.c_str());
                        spdlog::info("body length = {}", request.body_len);
                        if (request.body_len != 0) {
                            session.state = Session::BODY;
                            break;
                        }
                    }
                    replyClient(conn);
                    session.state = Session::REQUEST;
                    haveMoreline = false;
                    break;
                }
                auto pos = line.find(": ");
                if (pos != line.npos) {
                    std::string key(line, 0, pos);
                    std::string value(line, pos + 2, line.size());
                    // spdlog::info("{}: {}", key, value);
                    request.headers.insert({key, value});
                } else {
                    spdlog::error("parse headers error");
                    conn->close();
                }
                break;
            }
            haveMoreline = false;
            break;
        } break;
        case Session::BODY: {
        } break;
        default:
            break;
        }
    }
}

void WebServer::replyClient(Connection::Ptr conn) {
    auto &session = sessions_[conn.get()];
    auto &request = session.request;
    Session::Response resp;
    std::swap(session.response, resp);
    auto &response = session.response;
    if (request.url == "/wrk") {
        session.response.msg = "HTTP/1.1 200 OK";
        session.response.headers = "\r\nContent-Type: text/plain\r\nContent-Length: 12\r\n\r\n";
        session.response.body = "hello world\n";
        conn->write(session.response.msg.c_str(), session.response.msg.size());
        conn->write(session.response.headers.c_str(), session.response.headers.size());
        conn->write(session.response.body.c_str(), session.response.body.size());
        spdlog::info("{} {} {}", session.request.method, session.request.url, session.response.msg);
    } else {
        if (request.url.back() == '/') {
            request.url.pop_back();
        }
        if (request.url.empty()) {
            request.url = "/index.html";
        }
        spdlog::debug("url = {}", request.url);
        auto path = prefix_ + request.url;
        spdlog::debug("file path = {}", path);
        struct stat st;
        std::string resp_header;
        size_t length = 0;
        if (stat(path.c_str(), &st)) {
            path = prefix_ + "404.html";
            if (!stat(path.c_str(), &st)) {
                length = st.st_size;
            }
            response.msg = "HTTP/1.1 404 NOT FOUND";
        } else {
            length = st.st_size;
            response.msg = "HTTP/1.1 200 OK";
        }
        response.headers = fmt::format("\r\nContent-Type: text/plain\r\nContent-Length: {}\r\n\r\n", length);
        ::write(conn->getFd(), response.msg.c_str(), response.msg.size());
        ::write(conn->getFd(), response.headers.c_str(), response.headers.size());
        if (length) {
            int fd = open(path.c_str(), O_RDONLY);
            sendfile(conn->getFd(), fd, 0, length);
            close(fd);
        }
        spdlog::info("{} {} {}", session.request.method, session.request.url, session.response.msg);
    }
}
