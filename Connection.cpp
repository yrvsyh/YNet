#include "Connection.hpp"

#include <unistd.h>

Connection::Connection(EventLoop *loop, int fd, EndPoint local, EndPoint peer) : fd_(fd), local_(local), peer_(peer) {
    channel_.reset(new Channel(loop, fd_));
    channel_->onRead([this] { doRead(); });
    channel_->onWrite([this] { doWrite(); });
    channel_->onError([this] {
        close();
        stateCallback_();
    });
    channel_->onClose([this] {
        state_ = Closed;
        stateCallback_();
    });
}

void Connection::close() {
    state_ = Closed;
    shutdown(fd_, SHUT_WR);
}

void Connection::doRead() {}

void Connection::doWrite() {}
