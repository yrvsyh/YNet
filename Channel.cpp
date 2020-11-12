#include "Channel.hpp"
#include "EventLoop.hpp"

#include <sys/epoll.h>

Channel::Channel(EventLoop *loop, int fd)
    : loop_(loop), fd_(fd), status_(kNew), events_(0), eventHandling_(false), addedToLoop_(false) {}

Channel::~Channel() {
    //    remove();
}

void Channel::update() {
    addedToLoop_ = true;
    loop_->updateChannel(this);
}

void Channel::remove() {
    loop_->removeChannel(this);
    addedToLoop_ = false;
}

void Channel::handlerEvent() {
    eventHandling_ = true;
    if ((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN)) {
        closeCallback_();
    }
    if (revents_ & (EPOLLERR)) {
        errorCallback_();
    }
    if (revents_ & (EPOLLIN | EPOLLPRI | EPOLLRDHUP)) {
        readCallback_();
    }
    if (revents_ & EPOLLOUT) {
        writeCallback_();
    }
    eventHandling_ = false;
}
