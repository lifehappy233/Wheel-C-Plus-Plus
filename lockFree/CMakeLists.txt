cmake_minimum_required(VERSION 3.22)
set(CMAKE_CXX_STANDARD 20)

project(lockFree)

add_executable(lockFree main.cpp src/reclaim.cpp)

target_include_directories(lockFree PRIVATE include)
