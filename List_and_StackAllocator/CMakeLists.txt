cmake_minimum_required(VERSION 3.21)
project(List)

set(CMAKE_CXX_STANDARD 17)

add_executable(main main.cpp)
add_executable(test stackallocator_test.cpp)