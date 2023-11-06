cmake_minimum_required(VERSION 3.22)
set(CMAKE_CXX_STANDARD 20)

project(wheel-c++ LANGUAGES CXX)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/build/bin)

add_subdirectory(metaPrograming)

add_subdirectory(lockFree)
