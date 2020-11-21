#include "Connection.hpp"

#include <spdlog/spdlog.h>
#include <unistd.h>

Connection::Connection(EventLoop *loop, int fd, EndPoint local, EndPoint peer)
    : loop_(loop), state_(Connected), fd_(fd), local_(local), peer_(peer) {
    channel_.reset(new Channel(loop, fd_));
    channel_->onRead([&] { doRead(); });
    channel_->onWrite([&] { doWrite(); });
    channel_->onError([&] {
        close();
        stateCallback_(shared_from_this());
    });
    channel_->onClose([&] {
        state_ = Closed;
        stateCallback_(shared_from_this());
    });
    channel_->setEvents(EPOLLIN);
}

Connection::~Connection() { channel_->remove(); }

void Connection::shutdown() {
    state_ = Closed;
    ::shutdown(fd_, SHUT_WR);
}

void Connection::close() {
    state_ = Closed;
    ::close(fd_);
}

void Connection::doRead() {
    int savedErrno;
    ssize_t n = readbuf_.readFd(fd_, savedErrno);
    if (n > 0) {
        spdlog::debug("read {} bytes data", n);
        readCallback_(shared_from_this(), &readbuf_);
    } else if (n == 0) {
        Ptr This(shared_from_this());
        close();
        stateCallback_(This);
    } else {
    }
}

void Connection::doWrite() {}
