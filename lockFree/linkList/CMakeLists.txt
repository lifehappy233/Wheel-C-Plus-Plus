cmake_minimum_required(VERSION 3.22)
set(CMAKE_CXX_STANDARD 20)

project(linkList)

set(BOOST_ROOT /opt/homebrew/Cellar/boost/1.83.0)

find_package(Boost 1.83.0 COMPONENTS system)

include_directories(${Boost_INCLUDE_DIRS})
add_executable(linkList main.cpp)
target_link_libraries(linkList ${Boost_LIBRARIES})
