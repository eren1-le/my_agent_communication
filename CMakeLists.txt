cmake_minimum_required(VERSION 3.15)

project(agent_communication VERSION 1.0.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
enable_testing()
# 查找依赖
find_package(PkgConfig REQUIRED)
pkg_check_modules(GRPC REQUIRED grpc++)
pkg_check_modules(PROTOBUF REQUIRED protobuf)
# 设置编译选项
set(CMKAE_CXX_FLAG "${CMAKE_CXX_FLAGS} -Wall -Wextra -02")

add_subdirectory(tests)
