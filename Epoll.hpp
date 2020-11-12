#pragma once

#include <map>
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
    std::map<int, Channel *> channels_;
    std::vector<struct epoll_event> events_;
};
