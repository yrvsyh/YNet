cmake_minimum_required(VERSION 3.5)

project(YNet LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_BUILD_TYPE Debug)

add_definitions(-DSPDLOG_FMT_EXTERNAL)
add_definitions(-DSPDLOG_COMPILED_LIB)

add_library(
  ynet STATIC
  Epoll.cpp
  EventLoop.cpp
  Channel.cpp
  ThreadPool.cpp
  Connection.cpp
  Server.cpp
  WebServer.cpp)

add_executable(main main.cpp)

target_link_libraries(main ynet pthread spdlog fmt)
