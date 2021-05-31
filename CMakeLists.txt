cmake_minimum_required(VERSION 3.15)
project(MiniWebServer)

set(CMAKE_CXX_STANDARD 11)
# C/C++运行库静态编译进可执行文件里
 set(CMAKE_EXE_LINKER_FLAGS "-static")   # 动态库静态链接库到 exe 中


add_executable(main src/main.cpp)
target_link_libraries(main wsock32.lib)

message("CMAKE_CURRENT_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}")

include_directories(./include/my_gtest1.10)
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/lib/my_gtest1.10)

add_executable(TestHttpRequest test/TestHttpRequest.cpp)
target_link_libraries(TestHttpRequest libgtest_gcc8.1.0.a)

add_executable(TestHttpResponse test/TestHttpResponse.cpp)
target_link_libraries(TestHttpResponse libgtest_gcc8.1.0.a wsock32.lib)
