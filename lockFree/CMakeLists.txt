cmake_minimum_required(VERSION 3.22)
set(CMAKE_CXX_STANDARD 20)

project(lockFree)

add_executable(queue queue_test.cpp src/reclaim.cpp)

add_executable(stack stack_test.cpp src/reclaim.cpp)

add_executable(test test.cpp src/reclaim.cpp)

add_executable(hash hash_test.cpp src/reclaim.cpp)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")

target_include_directories(queue PRIVATE include)
target_include_directories(stack PRIVATE include)
target_include_directories(test PRIVATE include)
target_include_directories(hash PRIVATE include)

