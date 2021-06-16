cmake_minimum_required(VERSION 3.15)
project(MiniWebServer)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_EXE_LINKER_FLAGS "-static")   # 动态库静态链接库到 exe 中

message("CMAKE_CURRENT_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}")

if(WIN32)
    message(STATUS "Now is windows")
elseif(UNIX)
    message(STATUS "Now is UNIX-like OS's.")
endif()


add_executable(main_v1 v1/main_v1.cpp)
target_link_libraries(main_v1 ws2_32.lib)

add_executable(main_v2 v2/main_v2.cpp)
target_link_libraries(main_v2 ws2_32.lib)

add_executable(main_v3 v3/main_v3.cpp)

add_executable(main_v4 v4/main_v4.cpp)
target_link_libraries(main_v4 ws2_32.lib)



add_executable(test test.cpp)
if(WIN32)
    target_link_libraries(test wsock32.lib)
elseif(UNIX)

endif()


