# A2A Protocol Library CMakeLists.txt
# Integrated from /home/ubuntu/a2a-cpp
# Preserving original a2a:: namespace

cmake_minimum_required(VERSION 3.15)


# Find dependencies
find_package(CURL REQUIRED)
find_package(Threads REQUIRED)

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)