cmake_minimum_required(VERSION 3.22)
set(CMAKE_CXX_STANDARD 20)

project(threadPool LANGUAGES CXX)

add_executable(threadPool main.cpp src/threadPool.cpp)

target_include_directories(threadPool PRIVATE include)
