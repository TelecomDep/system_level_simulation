cmake_minimum_required(VERSION 3.22)
project(MobileSystem)
set(CMAKE_C_STANDARD 99)

add_executable(broker zmq_broker/broker.cpp)
target_link_libraries(broker zmq pthread)




