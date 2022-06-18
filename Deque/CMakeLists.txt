cmake_minimum_required(VERSION 3.21)
project(Deque)

set(CMAKE_CXX_STANDARD 17)

add_executable(Deque my_test.cpp deque.h mes_test.cpp)
add_executable(mes_test mes_test.cpp)
add_executable(test test.cpp)
