cmake_minimum_required(VERSION 3.20)

# MCP模块配置
set(MCP_SOURCES
    src/rag/embedding_service.cpp
    src/rag/embedding_cache.cpp)

add_library(agent_rpc_mcp STATIC)

# 设置包含目录
target_include_directories(agent_rpc_mcp PUBLIC
    include
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)


find_package(PkgConfig REQUIRED)
pkg_check_modules(JSONCPP REQUIRED jsoncpp)

# 链接依赖库
target_link_libraries(agent_rpc_mcp PRIVATE
    agent_rpc_common
    ${JSONCPP_LIBRARIES}
    Threads::Threads
    CURL::libcurl
)
target_include_directories(agent_rpc_mcp PRIVATE ${JSONCPP_INCLUDE_DIRS})

# 设置C++标准
target_compile_features(agent_rpc_mcp PRIVATE cxx_std_20)


# 安装规则
install(TARGETS agent_rpc_mcp
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY include/agent_rpc/mcp/
    DESTINATION include/agent_rpc/mcp
    FILES_MATCHING PATTERN "*.h"
)
