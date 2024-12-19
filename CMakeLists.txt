cmake_minimum_required(VERSION 3.22)
project(MobileSystem)
set(CMAKE_C_STANDARD 99)

add_executable(tcp_proxy src/tcp_proxy.cpp)
target_link_libraries(tcp_proxy zmq pthread)




