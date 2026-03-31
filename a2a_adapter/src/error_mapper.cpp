#include "agent_rpc/a2a_adapter/error_mapper.h"

namespace agent_rpc {
namespace a2a_adapter {

grpc::StatusCode ErrorMapper::mapToGrpcStatus(a2a::ErrorCode a2a_code) {
    switch (a2a_code) {
        case a2a::ErrorCode::ParseError:
            return grpc::StatusCode::INVALID_ARGUMENT;
            
        case a2a::ErrorCode::InvalidRequest:
            return grpc::StatusCode::INVALID_ARGUMENT;
            
        case a2a::ErrorCode::MethodNotFound:
            return grpc::StatusCode::UNIMPLEMENTED;
            
        case a2a::ErrorCode::InvalidParams:
            return grpc::StatusCode::INVALID_ARGUMENT;
            
        case a2a::ErrorCode::InternalError:
            return grpc::StatusCode::INTERNAL;
            
        case a2a::ErrorCode::TaskNotFound:
            return grpc::StatusCode::NOT_FOUND;
            
        case a2a::ErrorCode::TaskNotCancelable:
            return grpc::StatusCode::FAILED_PRECONDITION;
            
        case a2a::ErrorCode::UnsupportedOperation:
            return grpc::StatusCode::UNIMPLEMENTED;
            
        case a2a::ErrorCode::ContentTypeNotSupported:
            return grpc::StatusCode::INVALID_ARGUMENT;
            
        case a2a::ErrorCode::PushNotificationNotSupported:
            return grpc::StatusCode::UNIMPLEMENTED;
            
        default:
            return grpc::StatusCode::UNKNOWN;
    }
}

grpc::Status ErrorMapper::createGrpcStatus(
    a2a::ErrorCode a2a_code,
    const std::string& message) {
    
    grpc::StatusCode grpc_code = mapToGrpcStatus(a2a_code);
    std::string error_message = message.empty() 
        ? getErrorDescription(a2a_code) 
        : message;
    
    return grpc::Status(grpc_code, error_message);
}

grpc::Status ErrorMapper::createGrpcStatus(
    a2a::ErrorCode a2a_code,
    const std::string& message) {
    
    grpc::StatusCode grpc_code = mapToGrpcStatus(a2a_code);
    std::string error_message = message.empty() 
        ? getErrorDescription(a2a_code) 
        : message;
    
    return grpc::Status(grpc_code, error_message);
}

std::string ErrorMapper::getErrorDescription(a2a::ErrorCode a2a_code) {
    return a2a::error_code_to_string(a2a_code);
}

grpc::StatusCode ErrorMapper::mapIntToGrpcStatus(int32_t error_code) {
    // Map integer error codes to A2A ErrorCode enum
    switch (error_code) {
        case -32700:
            return mapToGrpcStatus(a2a::ErrorCode::ParseError);
        case -32600:
            return mapToGrpcStatus(a2a::ErrorCode::InvalidRequest);
        case -32601:
            return mapToGrpcStatus(a2a::ErrorCode::MethodNotFound);
        case -32602:
            return mapToGrpcStatus(a2a::ErrorCode::InvalidParams);
        case -32603:
            return mapToGrpcStatus(a2a::ErrorCode::InternalError);
        case -32001:
            return mapToGrpcStatus(a2a::ErrorCode::TaskNotFound);
        case -32002:
            return mapToGrpcStatus(a2a::ErrorCode::TaskNotCancelable);
        case -32003:
            return mapToGrpcStatus(a2a::ErrorCode::UnsupportedOperation);
        case -32004:
            return mapToGrpcStatus(a2a::ErrorCode::ContentTypeNotSupported);
        case -32005:
            return mapToGrpcStatus(a2a::ErrorCode::PushNotificationNotSupported);
        default:
            return grpc::StatusCode::UNKNOWN;
    }
}

} // namespace a2a_adapter
} // namespace agent_rpc