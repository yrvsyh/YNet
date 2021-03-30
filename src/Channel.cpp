#include "Channel.hpp"
#include "EventLoop.hpp"

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
    if (eventCb_) {
        eventCb_(revents_);
        return;
    }
    if ((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN)) {
        closeCb_();
    }
    if (revents_ & (EPOLLERR)) {
        errorCb_();
    }
    if (revents_ & (EPOLLIN | EPOLLPRI | EPOLLRDHUP)) {
        readCb_();
    }
    if (revents_ & EPOLLOUT) {
        writeCb_();
    }
    eventHandling_ = false;
}
