/*
 * @Author: eren dengdengd1222@mail.com
 * @Date: 2026-03-14 10:36:25
 * @LastEditors: eren dengdengd1222@mail.com
 * @LastEditTime: 2026-03-16 08:58:40
 * @FilePath: /my_agent_communication/include/agent_rpc/rpc_framework.h
 * @Description: 
 * 
 */
#pragma once
#include <mutex>
#include <memory>
#include <vector>
#include <map>
#include <functional>
#include <thread>
#include <atomic>
#include <grpcpp/health_check_service_interface.h>

namespace agent_rpc {

// 配置结构
struct RpcConfig {
    std::string server_address = "0.0.0.0:50051";
    int max_massage_size = 4 * 1024 * 1024; // 4MB
    int max_receive_message_size = 4 * 1024 * 1024; // 4MB
    int timeout_seconds  = 30;
    int max_retry_attempts = 3;
    int heartbeat_interval = 30;
    bool enable_ssl = false;
    std::string ssl_cert_path;
    std::string ssl_key_path;
    std::string log_level = "INFO";
    std::string register_address = "localhost:8500";
};

//服务信息
struct ServiceEndpoint {
    std::string host;
    int port;
    std::string service_name;
    std::string version;
    std::map<std::string, std::string> metadata;
    bool is_healthy = true;
    std::chrono::steady_clock::time_point last_heartbeat;
};

} // namespace agent_rpc


