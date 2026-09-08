cmake_minimum_required(VERSION 3.15)
project(OSAgent)

set(CMAKE_CXX_STANDARD 17)

add_executable(OSAgent main.cpp)
target_compile_definitions(OSAgent PRIVATE UNICODE _UNICODE)