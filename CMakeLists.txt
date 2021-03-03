cmake_minimum_required(VERSION 3.5)

project(YNet LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)                                                                                                                                                                         
set(CMAKE_BUILD_TYPE Debug)

add_definitions(-DSPDLOG_FMT_EXTERNAL)
add_definitions(-DSPDLOG_COMPILED_LIB)

add_executable(YNet           main.cpp
               EventLoop.cpp  EventLoop.hpp
               Epoll.cpp      Epoll.hpp
               Channel.cpp    Channel.hpp
               ThreadPool.cpp ThreadPool.hpp
               Connection.cpp Connection.hpp
               Buffer.hpp
               Server.cpp     Server.hpp
               WebServer.cpp  WebServer.hpp
               )

target_link_libraries(YNet pthread spdlog fmt)
