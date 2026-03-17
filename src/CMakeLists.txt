# 设置源文件列表
set(SOYRCES
    logger.cpp
    circuit_breaker.cpp
    load_balaner.cpp
    rpc_framework.cpp
    metric.cpp
)

# 创建静态库
add_library(agent_rpc_frame STATIC ${SOURCES})


# 连接依赖
target_link_libraries(agent_rpc_frame
    pthread
    proto_lib
)
# 指定目标的头文件搜索路径
target_include_directories(agent_rpc_frame PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../proto
)

# 安装规则
install(TARGETS agent_rpc_frame
    ARCHIVE DESTINATION lib # 安装动态库
    LIBRARY DESTINATION lib # 安装静态库
)

target_compile_options(agent_rpc_frame PRIVATE
    -Wall # 开启所有警告
    -Wextra # 开启额外警告
    -std=c++17
)
# 从目录中下载头文件到usr/include中，编译器默认在这个目录中查找
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../include/agent_rpc
    DESTINATION include
    FILES_MATCHING PATTERN "*.h"
)