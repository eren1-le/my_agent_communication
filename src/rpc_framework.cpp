/*
 * @Author: eren dengdengd1222@mail.com
 * @Date: 2026-03-14 10:37:12
 * @LastEditors: eren dengdengd1222@mail.com
 * @LastEditTime: 2026-03-20 10:06:19
 * @FilePath: /my_agent_communication/src/rpc_framework.cpp
 * @Description: 
 * 
 */
#include "agent_rpc/rpc_framework.h"
#include "agent_rpc/rpc_server.h"
#include "agent_rpc/rpc_client.h"
#include "agent_rpc/service_registry.h"
#include "agent_rpc/load_balancer.h"
#include "agent_rpc/logger.h"
#include "agent_rpc/metrics.h"

namespace agent_rpc{
RpcFramework& RpcFramework::getInstance() {
    static RpcFramework instance;
    return instance;
}


bool RpcFramework::initialize(const RpcConfig& config) {
    config_ = config;

    // 初始化日志
    LogConfig log_config;
    log_config.level = (config.log_level == "TRACE") ? LogLevel::TRACE :
                      (config.log_level == "DEBUG") ? LogLevel::DEBUG :
                      (config.log_level == "INFO") ? LogLevel::INFO :
                      (config.log_level == "WARN") ? LogLevel::WARN :
                      (config.log_level == "ERROR") ? LogLevel::ERROR : LogLevel::FATAL;
    log_config.console_output = true;
    log_config.file_output = false;
    log_config.async_logging = true;

    
    Logger::getInstance().initialize(log_config);

    LOG_INFO("Initializing RPC Framework with config: " + config.server_address);

    // 初始化监控指标
    
    Metrics::getInstance().initialize();

    // 初始化服务注册中心
    if (config.registry_address.find("consul") != std::string::npos) {
        registry_ = std::make_shared<ConsulServiceRegistry>();
    } else if (config.registry_address.find("etcd") != std::string::npos) {
        registry_ = std::make_shared<EtcdServiceRegistry>();
    } else {
        registry_ = std::make_shared<MemoryServiceRegistry>();
    }


    // 初始化负载均衡器
    load_balancer_ = LoadBalancerFactory::createLoadBalancer(LoadBalanceStrategy::ROUND_ROBIN);

    // 初始化服务器

    server_ = std::make_shared<RpcServer>();
    if (!server_->initialize(config)) {
        LOG_ERROR("Failed to initialize RPC server");
        return false;
    }

    // 初始化客户端
    client_ = std::make_shared<RpcClient>();
    if (!client_->initialize(config)) {
        LOG_ERROR("Failed to initialize RPC client");
        return false;
    }
    LOG_INFO("RPC Framework initialized successfully");
    return true;

}

bool RpcFramework::startServer() {
    if (running_) {
        LOG_WARN("RPC server is already running");
        return true;
    }
    
    if (!server_) {
        LOG_ERROR("RPC server not initialized");
        return false;
    }
    
    if (!server_->start()) {
        LOG_ERROR("Failed to start RPC server");
        return false;
    }
    
    running_ = true;
    LOG_INFO("RPC server started successfully on " + config_.server_address);
    return true;
}

void RpcFramework::stopServer() {
    if (!running_) {
        LOG_WARN("RPC server is not running");
        return;
    }
    
    if (server_) {
        server_->stop();
    }
    
    running_ = false;
    LOG_INFO("RPC server stopped");
}

std::shared_ptr<RpcServer> RpcFramework::getServer() {
    return server_;
}

std::shared_ptr<RpcClient> RpcFramework::getClient() {
    return client_;
}

std::shared_ptr<ServiceRegistry> RpcFramework::getRegistry() {
    return registry_;
}

std::shared_ptr<LoadBalancer> RpcFramework::getLoadBalancer() {
    return load_balancer_;
}





    
} // namespace agent_rpc