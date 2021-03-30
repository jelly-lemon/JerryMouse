cmake_minimum_required(VERSION 3.15)
project(MiniWebServer)

set(CMAKE_CXX_STANDARD 11)

add_executable(main main.cpp)
add_executable(MiniWebServer MiniWebServer.cpp)