#include "Server.hpp"
#include "NetUtil.hpp"

#include <cstdlib>
#include <cstring>
#include <errno.h>
#include <spdlog/spdlog.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>

Server::Server(EventLoop *loop, std::string ip, int port) : loop_(loop), endpoint_(ip, port) {
    listenFd_ = ::socket(AF_INET, SOCK_STREAM, 0);
    if (listenFd_ < 0) {
        spdlog::critical("can not create listen socket");
    }
    spdlog::debug("listenFd_ = {}", listenFd_);
    // setNonBlock(listenFd_);
    setReuseAddr(listenFd_);
}

void Server::start(int threadNum) {
    wokers_.resize(threadNum);
    int ret = ::bind(listenFd_, reinterpret_cast<const sockaddr *>(endpoint_.getAddr()), sizeof(sockaddr));
    if (ret < 0) {
        spdlog::critical("bind error");
    }
    ret = ::listen(listenFd_, 25);
    if (ret < 0) {
        spdlog::critical("listen error");
    }
    listenChannel_.reset(new Channel(loop_, listenFd_));
    listenChannel_->onRead([&] { newConn(); });
    listenChannel_->setEvents(EPOLLIN);
}

void Server::newConn() {
    sockaddr addr;
    std::memset(&addr, 0, sizeof(addr));
    socklen_t len = sizeof(addr);
    int fd = ::accept(listenFd_, &addr, &len);
    if (fd < 0) {
        spdlog::error("accept error: {}", strerror(errno));
    } else {
        setNonBlock(fd);
        EndPoint peer(&addr);
        spdlog::debug("peer: {}", peer.toString());
        auto loop = loop_;
        if (wokers_.size() > 0) {
            static int workerIndex = 0;
            workerIndex %= wokers_.size();
            loop = wokers_[workerIndex].getLoop();
            workerIndex++;
        }
        auto conn = std::make_shared<Connection>(loop, fd, endpoint_, peer);
        conns_.insert({peer.toString(), conn});
        newConnCallback_(conn);
        conn->onRead(msgCallback_);
        conn->onState([&](Connection::Ptr conn) { stateChanged(conn); });
    }
}

void Server::stateChanged(Connection::Ptr conn) {
    if (conn->getState() == Connection::Closed) {
        conns_.erase(conn->getPeer().toString());
        closeCallback_(conn);
    }
}
