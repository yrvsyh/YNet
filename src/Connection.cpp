#include "Connection.hpp"
#include "EventLoop.hpp"

#include <spdlog/spdlog.h>
#include <unistd.h>

Connection::Connection(EventLoop *loop, int fd, EndPoint local, EndPoint peer)
    : loop_(loop), state_(Connected), fd_(fd), local_(local), peer_(peer), ctx_(new AutoContext()) {
    channel_.reset(new Channel(loop, fd_));
    channel_->onRead([this] { doRead(); });
    channel_->onWrite([this] { doWrite(); });
    channel_->onError([this] { doClose(); });
    channel_->onClose([this] { doClose(); });
    channel_->setEvents((0u - 1) & (EPOLLIN | EPOLLOUT));
}

Connection::~Connection() {
    // spdlog::trace("Connection[{}: {}] Dtor", fd_, peer_.toString());
}

void Connection::shutdown() {
    state_ = Closed;
    ::shutdown(fd_, SHUT_WR);
}

void Connection::close() {
    channel_->setEvents(0);
    channel_->remove();
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
        SPDLOG_DEBUG("read {} bytes data", n);
        readCallback_(shared_from_this(), &readbuf_);
    } else if (n == 0) {
        doClose();
    } else {
        doClose();
    }
}

void Connection::doWrite() {
    if (state_ == Connected && writebuf_.readableBytes() > 0) {
        SPDLOG_DEBUG("writing {} bytes data to {}", writebuf_.readableBytes(), peer_.toString());
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

void Connection::doClose() {
    ConnectionPtr This(shared_from_this());
    close();
    loop_->queueInLoop([this, This] { closeCallback_(This); });
}
