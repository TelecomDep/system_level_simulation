cmake_minimum_required(VERSION 3.5)
project(MobileSystem)
set(CMAKE_C_STANDARD 99)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0")
set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} -g -O0")

# find_package(nlohmann_json 3.11.3 REQUIRED) # to serialize from Json to C++ object
add_subdirectory(third_party/json)


add_executable(broker
    zmq_broker/old_broker.cpp
    zmq_broker/source/Equipment.cpp
    zmq_broker/source/Broker.cpp
    zmq_broker/source/gNB.cpp
    zmq_broker/source/UE.cpp
    zmq_broker/source/subfuncs.cpp
)

add_executable(matlab_zmq_test
    zmq_broker/tests/matlab_zmq.cpp
)

target_link_libraries(broker zmq pthread boost_program_options) 
target_link_libraries(broker nlohmann_json::nlohmann_json)
target_include_directories(broker PRIVATE includes)

target_link_libraries(matlab_zmq_test zmq pthread boost_program_options) 