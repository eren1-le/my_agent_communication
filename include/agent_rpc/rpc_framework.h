/*
 * @Author: eren dengdengd1222@mail.com
 * @Date: 2026-03-14 10:36:25
 * @LastEditors: eren dengdengd1222@mail.com
 * @LastEditTime: 2026-03-18 18:54:11
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
#include "logger.h"
#include "metric.h"
#include "proto/agent_service.grpc.pb.h"
namespace agent_rpc {
// 前向声明
class Metrics;
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

// 消息队列
template<typename T> 
class MessageQueue {
public:
    void push(const T& item) {
        std::lock_guard<std::mutex> locker(mutex_);
        queue_.push(item);
        condition_.notify_one();
    }

    bool try_pop(T& item, std::chrono::milliseconds timeout = std::chrono::milliseconds(0)) {
        std::unique_lock<std::mutex> locker(mutex_);
        if (condition_.wait_for(locker, timeout, [this] { return !queue_.empty(); })) {
            item = queue_.front();
            queue_.pop();
            return true;
        }
        return false;
    }

    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.size();
    }

    bool empty() const {
        std::lock_guard<std::mutex> locker(mutex_);
        return queue_.empty();
    }

private:
    mutable std::mutex mutex_;
    std::queue<T> queue_;
    std::condition_variable condition_;
};

// 回调函数类型定义
using MessageHandler = std::function<void(const agent_communication::Message &)>;

using ErrorHandler = std::function<void(const std::string&, int)>;
using HealthCheckHandler = std::function<bool()>;
} // namespace agent_rpc


