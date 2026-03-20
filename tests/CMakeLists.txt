# 查找Google Test

find_package(GTest REQUIRED)


add_executable(test_rpc_framework test_rpc_framework.cpp)

target_link_libraries(test_rpc_framework
    proto_lib
    GTest::gtest
    GTest::gtest_main
    pthread
    uuid
    agent_rpc_framework
)   
target_include_directories(test_rpc_framework PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}/../proto
    ${CMAKE_CURRENT_SOURCE_DIR}/../include    
)   
enable_testing()

add_test(NAME RpcframeworkTest COMMAND test_rpc_framework)

