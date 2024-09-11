cmake_minimum_required(VERSION 3.22)
project(MobileSystem)
set(CMAKE_C_STANDARD 99)
find_package(PkgConfig REQUIRED)
pkg_check_modules(PC_ZMQ REQUIRED libzmq)

add_executable(client ./tests/client.c)
target_link_libraries(client ${PC_ZMQ_LIBRARIES})
add_executable(server ./tests/server.c)

target_link_libraries(server ${PC_ZMQ_LIBRARIES})
target_include_directories(client PUBLIC ${PC_ZMQ_INCLUDE_DIRS})
target_include_directories(server PUBLIC ${PC_ZMQ_INCLUDE_DIRS})




