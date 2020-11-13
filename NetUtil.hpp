#pragma once

#include <arpa/inet.h>
#include <cstring>
#include <string>

class EndPoint {
public:
    EndPoint(sockaddr *sa, int len) {
        std::memcpy(&sin_, sa, len);
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
