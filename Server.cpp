#include "Server.hpp"
#include "EventLoop.hpp"
#include "Utils.hpp"

#include <cstdlib>
#include <cstring>
#include <errno.h>
#include <spdlog/spdlog.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

Server::Server(EventLoop *loop, std::string ip, int port) : loop_(loop), endpoint_(ip, port) {
    listenFd_ = ::socket(AF_INET, SOCK_STREAM, 0);
    spdlog::critical(listenFd_ < 0, "can not create listen socket");
    SPDLOG_DEBUG("listenFd_ = {}", listenFd_);
    setReuseAddr(listenFd_);
}

Server::~Server() {
    for (auto &worker : workers_) {
        SPDLOG_DEBUG("quiting worker");
        worker->getLoop()->quit();
        worker->join();
    }
}

void Server::start(int threadNum, int maxConn) {
    workerNums_ = threadNum;
    maxConn_ = maxConn;
    workers_.resize(workerNums_);
    for (auto &worker : workers_) {
        worker.reset(new EventThread());
    }
    int ret = ::bind(listenFd_, reinterpret_cast<const sockaddr *>(endpoint_.getAddr()), sizeof(sockaddr));
    spdlog::critical(ret < 0, "bind error");
    ret = ::listen(listenFd_, 25);
    spdlog::critical(ret < 0, "listen error");
    listenChannel_.reset(new Channel(loop_, listenFd_));
    listenChannel_->onRead([this] { newConn(); });
    listenChannel_->enableRead(true);
}

void Server::newConn() {
    sockaddr addr;
    std::memset(&addr, 0, sizeof(addr));
    socklen_t len = sizeof(addr);
    int fd = ::accept(listenFd_, &addr, &len);
    if (fd < 0) {
        spdlog::error("accept error: {}", strerror(errno));
    } else if (conns_.size() > maxConn_) {
        spdlog::error("too many clients, close {}", EndPoint(&addr).toString());
        ::close(fd);
    } else {
        setNonBlock(fd);
        EndPoint peer(&addr);
        SPDLOG_DEBUG("peer: {}", peer.toString());
        auto loop = loop_;
        if (workerNums_ > 0) {
            static int workerIndex = 0;
            workerIndex %= workerNums_;
            loop = workers_[workerIndex]->getLoop();
            workerIndex++;
        }
        auto conn = std::make_shared<Connection>(loop, fd, endpoint_, peer);
        conns_.insert(conn);
        connCb_(conn);
        conn->onRead(readCb_);
        conn->onClose([this](ConnectionPtr conn) { closeConn(conn); });
        conn->onWrite([](ConnectionPtr conn) { SPDLOG_DEBUG("write to {} done", conn->getPeer().toString()); });
        conn->enableRead(true);
    }
}

void Server::closeConn(ConnectionPtr conn) {
    // 在Server主线程完成Connection的移除操作
    loop_->runInLoop([this, conn] {
        conns_.erase(conn);
        closeCb_(conn);
    });
}
