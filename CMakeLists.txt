cmake_minimum_required(VERSION 3.22)
project(MobileSystem)
set(CMAKE_C_STANDARD 99)

add_executable(zmq_broker
    zmq_broker/main.cpp
    zmq_broker/source/Broker.cpp
    zmq_broker/source/gNB.cpp
    zmq_broker/source/UE.cpp
)

target_link_libraries(zmq_broker zmq pthread boost_program_options) 


target_include_directories(zmq_broker PRIVATE includes)