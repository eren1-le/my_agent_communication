
# 生成protobuf和gRPC代码
set(PROTO_FILES
    common.proto
    agent_service.proto
    ai_query.proto
)


foreach(PROTO_FILE ${PROTO_FILES})
    # 提取文件名 （不包含扩展）
    get_filename_component(PROTO_NAME ${PROTO_FILE} NAME_WE)
    # 生成protobuf代码
    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${PROTO_NAME}.pb.cc # 输出的源文件
               ${CMAKE_CURRENT_BINARY_DIR}/${PROTO_NAME}.pb.h # 输出的头文件
        COMMAND protoc # 执行protoc命令
        ARGS --cpp_out=${CMAKE_CURRENT_BINARY_DIR} # 输出目录
             --proto_path=${CMAKE_CURRENT_SOURCE_DIR} # proto文件搜索路径
             ${CMAKE_CURRENT_SOURCE_DIR}/${PROTO_FILE} # 输入的proto文件
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${PROTO_FILE} # 依赖关系
        COMMENT "Generating protobuf code for ${PROTO_FILE}" # 输出信息
    )
    # 生成gRPC代码
    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${PROTO_NAME}.grpc.pb.cc
               ${CMAKE_CURRENT_BINARY_DIR}/${PROTO_NAME}.grpc.pb.h
        COMMAND protoc 
        ARGS  --grpc_out=${CMAKE_CURRENT_BINARY_DIR}
              --plugin=protoc-gen-grpc=/usr/bin/grpc_cpp_plugin # 生成gRPC相关的c++代码
              --proto_path=${CMAKE_CURRENT_SOURCE_DIR}
              ${CMAKE_CURRENT_SOURCE_DIR}/${PROTO_FILE}
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${PROTO_FILE}
        COMMENT "Generating gRPC code for ${PROTO_FILE}"
    )
    # 添加元素
    list(APPEND PROTO_SOURCES
        ${CMAKE_CURRENT_BINARY_DIR}/${PROTO_NAME}.pb.cc
        ${CMAKE_CURRENT_BINARY_DIR}/${PROTO_NAME}.grpc.pb.cc
        )
    list(APPEND PROTO_HEADERS
        ${CMAKE_CURRENT_BINARY_DIR}/${PROTO_NAME}.pb.h
        ${CMAKE_CURRENT_BINARY_DIR}/${PROTO_NAME}.grpc.pb.h
    )  
endforeach()

# 创建proto库

add_library(proto_lib STATIC ${PROTO_SOURCES} ${PROTO_HEADERS})

target_link_libraries(proto_lib ${GRPC_LIBRARIES} ${PROTOBUF_LIBRARIES})
target_include_directories(proto_lib PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

# 导出头文件
install(FILES ${PROTO_HEADERS} DESTINATION include/proto)


