#pragma once

#include <arpa/inet.h>
#include <cstring>
#include <fcntl.h>
#include <string>

class EndPoint {
public:
    EndPoint(sockaddr *sa) {
        std::memset(&sin_, 0, sizeof(sin_));
        std::memmove(&sin_, sa, sizeof(sockaddr_in));
        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &sin_.sin_addr, ip, sizeof(ip));
        ip_ = ip;
        port_ = ntohs(sin_.sin_port);
    }
    EndPoint(std::string ip, int port) : ip_(ip), port_(port) {
        std::memset(&sin_, 0, sizeof(sin_));
        sin_.sin_family = AF_INET;
        sin_.sin_addr.s_addr = inet_addr(ip_.c_str());
        sin_.sin_port = htons(port);
    }
    sockaddr_in *getAddr() { return &sin_; }
    std::string toString() { return ip_ + ":" + std::to_string(port_); }

private:
    sockaddr_in sin_;
    std::string ip_;
    int port_;
};

inline void setNonBlock(int fd) {
    int flags = ::fcntl(fd, F_GETFL, 0);
    flags |= O_NONBLOCK;
    ::fcntl(fd, F_SETFL, flags);
}

inline void setReuseAddr(int fd) {
    int optval = 1;
    ::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &optval, static_cast<socklen_t>(sizeof optval));
}
