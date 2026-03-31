# 查找Google Test

find_package(GTest REQUIRED)

# MCP Agent集成测试
# **Feature: a2a-integration, Task 20: MCP 集成测试**
# **Validates: Requirements 12.1, 12.3, 12.5**
add_executable(test_mcp_integration test_mcp_integration.cpp)
target_link_libraries(test_mcp_integration 
    agent_rpc_mcp
    agent_rpc_common
    rapidcheck
    GTest::gtest
    GTest::gtest_main
    pthread
)

target_include_directories(test_mcp_integration PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../mcp/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../common/include
)

add_test(NAME MCPIntegrationTest COMMAND test_mcp_integration)