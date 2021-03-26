#pragma once

#include "Utils.hpp"

#include <sys/epoll.h>
#include <unordered_set>
#include <vector>

class Channel;

class Epoll {
public:
    Epoll();
    ~Epoll();
    void updateChannel(Channel *channel);
    void removeChannel(Channel *channel);
    void wait(std::vector<Channel *> &activeChannels, int timeout);

private:
    void update(int operation, Channel *channel);

private:
    int epfd_;
    std::unordered_set<Channel *> channels_;
    std::vector<epoll_event> events_;
};
