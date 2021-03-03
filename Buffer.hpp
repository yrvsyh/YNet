#pragma once

#include <cstring>
#include <errno.h>
#include <spdlog/spdlog.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <vector>

class Buffer {
public:
    Buffer() : buffer_(1024), start_(0), end_(0) {}
    size_t readableBytes() const { return end_ - start_; }
    size_t writableBytes() const { return buffer_.size() - end_; }
    const char *peek() { return buffer_.data() + start_; }
    void append(const char *data, size_t len) {
        if (len < writableBytes()) {
            std::memmove(buffer_.data() + end_, data, len);
            end_ += len;
        } else if (buffer_.size() - end_ + start_ < len) {
            std::memmove(buffer_.data(), buffer_.data() + start_, end_ - start_);
            end_ = end_ - start_;
            start_ = 0;
            std::memmove(buffer_.data() + end_, data, len);
            end_ += len;
        } else {
            buffer_.resize(buffer_.size() + len);
            std::memmove(buffer_.data() + end_, data, len);
            end_ += len;
        }
    }
    void retieve(size_t len) {
        if (len < readableBytes()) {
            start_ += len;
        } else {
            start_ = end_ = 0;
        }
    }
    std::string readUntil(const char *str, bool &ok) {
        const char *pos = std::search(peek(), peek() + readableBytes(), str, str + strlen(str));
        if (pos) {
            std::string ret(peek(), pos);
            retieve(pos + strlen(str) - peek());
            ok = true;
            return ret;
        }
        ok = false;
        return std::string();
    }
    ssize_t readFd(int fd, int &savedErrno) {
        static char extrabuf[65536];
        struct iovec vec[2];
        const size_t writable = writableBytes();
        vec[0].iov_base = buffer_.data() + end_;
        vec[0].iov_len = writable;
        vec[1].iov_base = extrabuf;
        vec[1].iov_len = sizeof(extrabuf);
        const int iovcnt = (writable < sizeof(extrabuf)) ? 2 : 1;
        const ssize_t n = ::readv(fd, vec, iovcnt);
        if (n < 0) {
            savedErrno = errno;
        } else if (n <= writable) {
            end_ += n;
        } else {
            end_ = buffer_.size();
            append(extrabuf, n - writable);
        }
        spdlog::trace("start_ = {}, end_ = {}", start_, end_);
        return n;
    }

private:
    std::vector<char> buffer_;
    size_t start_;
    size_t end_;
};
