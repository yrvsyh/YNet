#include "Epoll.hpp"
#include "Channel.hpp"
#include "Utils.hpp"

#include <cerrno>
#include <spdlog/spdlog.h>
#include <unistd.h>

Epoll::Epoll() {
    epfd_ = ::epoll_create1(EPOLL_CLOEXEC);
    events_.reserve(16);
}

Epoll::~Epoll() { ::close(epfd_); }

void Epoll::updateChannel(Channel *channel) {
    SPDLOG_DEBUG("update channel {}", channel->fd());
    if (channel->status() == Channel::kNew) {
        channels_.insert(channel);
        channel->setStatus(Channel::kAdded);
        update(EPOLL_CTL_ADD, channel);
    } else if (channel->status() == Channel::kAdded) {
        update(EPOLL_CTL_MOD, channel);
    }
}

void Epoll::removeChannel(Channel *channel) {
    SPDLOG_DEBUG("remove channel {}", channel->fd());
    if (channel->status() == Channel::kAdded) {
        channels_.erase(channel);
        update(EPOLL_CTL_DEL, channel);
        channel->setStatus(Channel::kDeleted);
    }
}

void Epoll::wait(std::vector<Channel *> &activeChannels, int timeout) {
    auto nfds = ::epoll_wait(epfd_, events_.data(), events_.capacity(), timeout);
    if (nfds > 0) {
        SPDLOG_TRACE("{} events happened", nfds);
        std::vector<Channel *> tmp;
        tmp.swap(activeChannels);
        activeChannels.reserve(nfds);
        for (int i = 0; i < nfds; i++) {
            auto channel = static_cast<Channel *>(events_[i].data.ptr);
            channel->setRevents(events_[i].events);
            activeChannels.push_back(channel);
        }
        if (static_cast<size_t>(nfds) == events_.size()) {
            events_.resize(events_.size() * 2);
        }
    } else if (nfds == 0) {
        SPDLOG_TRACE("nothing happened in {} millisecond", timeout);
    } else {
        spdlog::critical(true, "epoll_wait error");
    }
}

void Epoll::update(int operation, Channel *channel) {
    struct epoll_event event;
    ::memset(&event, 0, sizeof(event));
    event.events = channel->events();
    event.data.ptr = channel;
    ::epoll_ctl(epfd_, operation, channel->fd(), &event);
}
