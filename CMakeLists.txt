cmake_minimum_required(VERSION 3.22)
project(MobileSystem)
set(CMAKE_C_STANDARD 99)
# find_package(PkgConfig REQUIRED)
# pkg_check_modules(PC_ZMQ REQUIRED libzmq pcap)


# execute_process(
#     # COMMAND sudo apt-get install libpcap-dev
# )



add_executable(client ./tests/client.c)
target_link_libraries(client zmq pcap)
add_executable(server ./tests/server.c)

target_link_libraries(server zmq pcap)

add_executable(live_capture ./tests/live_capture.c)
target_link_libraries(live_capture zmq pcap)


# target_include_directories(client PUBLIC ${PC_ZMQ_INCLUDE_DIRS})
# target_include_directories(server PUBLIC ${PC_ZMQ_INCLUDE_DIRS})




