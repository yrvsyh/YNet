# GNU/Linux平台下基于Epoll的高性能现代C++网络库

## 项目结构

### Epoll类

封装epoll相关调用，负责更新删除Channel以及epoll_wait系统调用。

### Channel类

将一个fd包装为一个Channel对象，设置各个事件发生时的回调函数供EventLoop分发。

### EventLoop类

主事件循环。负责事件的分发，提供了runInLoop函数进行可保证任务在loop线程中调用。
提供对信号的响应，Ctrl-C时可安全退出。

### Connection类

将一个Tcp连接包装为一个Connection对象，继承至std::enable_shared_from_this，自动管理该对象的生命期。结合Buffer负责具体的读写任务、连接关闭或错误处理。

### Buffer类

使用std::vector<char>作为存储单元，内部使用了start、end两个指针控制可读可写的范围。

读数据时在栈上分配较大的空间，配合readv系统调用实现分散读取，高效读取大小数据。

### Server类

包含0个或多个worker线程负责连接的IO处理。

主线程创建用于接收连接的listenSocket，处理新建立的连接并通过轮询的方式进行分发。

Server类维护一个Connction的set，新连接建立时插入，连接关闭或发生错误时closeConn方法被调用，使用EventLoop提供的runInLoop函数转至**主线程**中安全的删除该Connection。若此时为该连接的最后一个引用，Connection便会自动销毁。

### WebServer类

提供了简易的Web服务器，使用状态机解析HTTP请求。

### ThreadPool

使用队列、互斥锁和条件变量实现了线程池，用于处理耗时任务。

初始化时需要屏蔽所有信号`sigprocmask(SIG_BLOCK, &mask, nullptr);`，否则会影响主线程对信号的捕获。

## 性能测试

使用wrk测试WebServer(8线程)，1000连接8线程时 QPS 5W以上。

## TODO

EventLoop添加定时任务