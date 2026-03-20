# 设置源文件列表
set(SOURCES
    logger.cpp
    circuit_breaker.cpp
    load_balancer.cpp
    rpc_framework.cpp
    metrics.cpp
    rpc_client.cpp
    rpc_server.cpp
    service_registry.cpp
)

# 创建静态库
add_library(agent_rpc_framework STATIC ${SOURCES})


# 连接依赖
target_link_libraries(agent_rpc_framework
    pthread
    proto_lib
    jsoncpp
    curl
    grpc++_reflection
)
# 指定目标的头文件搜索路径
target_include_directories(agent_rpc_framework PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${CMAKE_CURRENT_BINARY_DIR}/../proto
    /usr/include/jsoncpp
)

# 安装规则
install(TARGETS agent_rpc_framework
    ARCHIVE DESTINATION lib # 安装动态库
    LIBRARY DESTINATION lib # 安装静态库
)

target_compile_options(agent_rpc_framework PRIVATE
    -Wall # 开启所有警告
    -Wextra # 开启额外警告
    -std=c++17
)
# 从目录中下载头文件到usr/include中，编译器默认在这个目录中查找
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../include/agent_rpc
    DESTINATION include
    FILES_MATCHING PATTERN "*.h"
)