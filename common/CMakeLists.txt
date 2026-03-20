cmake_minimum_required(VERSION 3.15)

set(COMMON_SOURCES
    src/logger.cpp
)

add_library(agent_rpc_common STATIC ${COMMON_SOURCES})

target_link_libraries(agent_rpc_common
    ${GRPC_LIBRARIES}
    ${PROTOBUF_LIBRARIES}
    pthread
    uuid
    curl
    jsoncpp
)

target_include_directories(agent_rpc_common PUBLIC
    ${CMAKE_CURRENT_BINARY_DIR}/../proto
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)


target_compile_options(agent_rpc_common PRIVATE
    -Wall
    -Wextra
    -std=c++17
)


# 安装规则
install(TARGETS agent_rpc_common
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/agent_rpc
    DESTINATION include
    FILES_MATCHING PATTERN "*.h"
)