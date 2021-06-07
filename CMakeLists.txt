cmake_minimum_required(VERSION 3.15)
project(MiniWebServer)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_EXE_LINKER_FLAGS "-static")   # 动态库静态链接库到 exe 中

message("CMAKE_CURRENT_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}")


add_executable(main_v1 v1/main_v1.cpp)
target_link_libraries(main_v1 wsock32.lib)

add_executable(main_v2 v2/main_v2.cpp)
target_link_libraries(main_v2 wsock32.lib)

