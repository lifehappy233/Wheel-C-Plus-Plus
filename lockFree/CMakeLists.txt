cmake_minimum_required(VERSION 3.22)
set(CMAKE_CXX_STANDARD 20)

project(lockFree)

add_executable(queue queue_test.cpp src/reclaim.cpp)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")

target_include_directories(queue PRIVATE include)
