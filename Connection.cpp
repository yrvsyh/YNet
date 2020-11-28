#include "Connection.hpp"

#include <spdlog/spdlog.h>
#include <unistd.h>

Connection::Connection(EventLoop *loop, int fd, EndPoint local, EndPoint peer)
    : loop_(loop), state_(Connected), fd_(fd), local_(local), peer_(peer) {
    channel_.reset(new Channel(loop, fd_));
    channel_->onRead([this] { doRead(); });
    channel_->onWrite([this] { doWrite(); });
    channel_->onError([this] {
        Ptr This(shared_from_this());
        close();
        closeCallback_(This);
    });
    channel_->onClose([this] {
        Ptr This(shared_from_this());
        close();
        closeCallback_(This);
    });
}

Connection::~Connection() {
    // spdlog::error("Connection[{}] Dtor", fd_);
    channel_->remove();
}

void Connection::shutdown() {
    state_ = Closed;
    ::shutdown(fd_, SHUT_WR);
}

void Connection::close() {
    channel_->setEvents(0);
    state_ = Closed;
    ::close(fd_);
}

void Connection::write(const char *data, size_t len) {
    if (len > 0) {
        writebuf_.append(data, len);
        channel_->enableWrite(true);
    }
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
        closeCallback_(This);
    } else {
    }
}

void Connection::doWrite() {
    if (state_ == Connected && writebuf_.readableBytes() > 0) {
        spdlog::debug("writing {} bytes data to {}", writebuf_.readableBytes(), peer_.toString());
        ssize_t n = ::write(fd_, writebuf_.peek(), writebuf_.readableBytes());
        if (n > 0) {
            writebuf_.retieve(n);
            if (writebuf_.readableBytes() == 0) {
                channel_->enableWrite(false);
                if (writeCallback_) {
                    writeCallback_(shared_from_this());
                }
            }
        }
    }
}
