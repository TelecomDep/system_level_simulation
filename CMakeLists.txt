cmake_minimum_required(VERSION 3.22)
project(MobileSystem)
set(CMAKE_C_STANDARD 99)


add_executable(client ./tests/client.c)
target_link_libraries(client zmq pcap)
add_executable(server ./tests/server.c)



target_link_libraries(server zmq pcap)

add_executable(live_capture ./tests/live_capture.c)
target_link_libraries(live_capture zmq pcap)


add_executable(tcp_proxy ./libmzmq/tcp_proxy.cpp)
target_link_libraries(tcp_proxy zmq pcap)
