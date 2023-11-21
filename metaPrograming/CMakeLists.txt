cmake_minimum_required(VERSION 3.22)

project(metaPrograming)

add_executable(tuple tuple.cpp)
add_executable(typeList type_list.cpp)
add_executable(variant variant.cpp)

target_include_directories(tuple PRIVATE include)
target_include_directories(typeList PRIVATE include)
target_include_directories(variant PRIVATE include)
