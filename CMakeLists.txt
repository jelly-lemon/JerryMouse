cmake_minimum_required(VERSION 3.15)
project(JerryMouse)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_EXE_LINKER_FLAGS "-static")   # 动态库静态链接库到 exe 中

message("CMAKE_CURRENT_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}")

if(WIN32)
    message(STATUS "Now is Windows")
elseif(UNIX)
    message(STATUS "Now is Linux")
endif()

include_directories(${PROJECT_SOURCE_DIR}/include)

add_executable(main_v0 v0_one_thread/main_v0.cpp)
if(WIN32)
    target_link_libraries(main_v0 ws2_32.lib)
endif()


add_executable(main_v1 v1_per_connection_per_thread/main_v1.cpp)
if(WIN32)
    target_link_libraries(main_v1 ws2_32.lib)
endif()

add_executable(main_v1_1 v1_1_threadpool/main_v1_1.cpp)
if(WIN32)
    target_link_libraries(main_v1_1 ws2_32.lib)
endif()

add_executable(main_v2 v2_select_threadpool/main_v2.cpp)
if(WIN32)
    target_link_libraries(main_v2 ws2_32.lib)
endif()


add_executable(main_v2_1 v2_1_poll_threadpool/main_v2_1.cpp)
if(WIN32)
    target_link_libraries(main_v2_1 ws2_32.lib)
endif()


add_executable(main_v2_2 v2_2_reactor/main_v2_2.cpp)
if(WIN32)
    target_link_libraries(main_v2_2 ws2_32.lib)
endif()

add_executable(main_v4_1 v4_1_acceptEx_IOCP_threadpool/main_v4_1.cpp)
if(WIN32)
    target_link_libraries(main_v4_1 ws2_32.lib)
endif()

add_executable(test test.cpp)
if(WIN32)
    target_link_libraries(test wsock32.lib)
elseif(UNIX)

endif()


