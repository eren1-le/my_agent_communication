/*""
 * @Author: eren dengdengd1222@mail.com
 * @Date: 2026-03-17 14:29:11
 * @LastEditors: eren dengdengd1222@mail.com
 * @LastEditTime: 2026-03-17 14:29:17
 * @FilePath: /my_agent_communication/src/service_register.cpp
 * @Description: 
 * 
 */
#include "agent_rpc/service_registry.h"
#include <curl/curl.h>
#include <json/json.h>
#include "agent_rpc/logger.h"
namespace agent_rpc {
// ConsulServiceRegister 实现
ConsulServiceRegistry::ConsulServiceRegistry() {
    curl_global_init(CURL_GLOBAL_DEFAULT); // 初始化libcurl全局环境
}

ConsulServiceRegistry::~ConsulServiceRegistry() {
    stopHealthCheck();
    curl_global_cleanup();  // 清理libcurl全局资源
}
bool ConsulServiceRegistry::initialize(const std::string& consul_address) {
    consul_address_ = consul_address;
    LOG_INFO("Consul service registry initialize with address" + consul_address);
    return true;
}

bool ConsulServiceRegistry::registerService(const ServiceEndpoint& endpoint) {
    // 生成唯一ID
    std::string service_id = getServiceId(endpoint);

    // 构建Consul服务注册JSON
    Json::Value service_json;
    service_json["ID"] = service_id;
    service_json["Name"] = endpoint.service_name;
    service_json["Address"] = endpoint.host;
    service_json["Port"] = endpoint.port;
    service_json["Tags"] = Json::Value(Json::arrayValue);  // 标签
    service_json["Meta"] = Json::Value(Json::objectValue); // 元数据

    // 添加自定义数据
     for (const auto& pair : endpoint.metadata) {
        service_json["Meta"][pair.first] = pair.second;
     }
    // 发送注册请求到Consul
     Json::StreamWriterBuilder builder; // 创建JSON构建器
     std::string json_string = Json::writeString(builder, service_json); // 将服务对象转换为JSON字符串

     std::string url = "http://" + consul_address_ + "/v1/agent/service/register"; // 构建Consul API的URL
     std::string response = makeHttpRequest("PUT", url, json_string); // 发送HTTP，PUT请求到Consul
     if (response.empty()) { // 检查响应结果
        LOG_ERROR("Failed to register service: " + service_id);
        return false;
     }
     // 保存到本地缓存
     {
        std::lock_guard<std::mutex> locker(service_mutex_);
        registered_services_[service_id] = endpoint;
     }

     LOG_INFO("Service register: " + service_id);
     return true;

}

bool ConsulServiceRegistry::unregisterService(const std::string& service_id) {
    std::string url = "http://" + consul_address_ +  "/v1/agent/service/deregister/" + service_id;
    std::string response = makeHttpRequest("PUT", url);

    {
        std::lock_guard<std::mutex> locker(service_mutex_);
        registered_services_.erase(service_id);
    }

    LOG_INFO("Service unregitered: " + service_id);
    return true;
}
std::vector<ServiceEndpoint> ConsulServiceRegistry::discoverServices( const std::string& service_name) {
    std::string url = "http://" + consul_address_ + "/v1/health/service/" + service_name;
    std::string response = makeHttpRequest("GET", url);
    if (response.empty()) {
        LOG_ERROR("Failed to discover services: " + service_name);
         return {};
    }
    std::vector<ServiceEndpoint> services = parseServiceList(response);
    // 缓存结果
    {
        std::lock_guard<std::mutex> lock(service_mutex_);
        discovered_services_[service_name] = services;
    }
    return services;
}

bool ConsulServiceRegistry::isServiceHealthy(const std::string& service_id) {
    std::string url = "http://" + consul_address_ + "/v1/agent/health/service/id/" + service_id;
    std::string response = makeHttpRequest("GET", url);

    if (response.empty()) {
        return false;
    }
    // 解析JSON响应
    Json::Value root;
    Json::Reader reader;
    if (reader.parse(response, root)) {
        // 检查状态是否为passing
        return root["Status"].asString() == "passing";
    }
    return false;
}

bool ConsulServiceRegistry::updateHeartbeat(const std::string& service_id) {
    std::string url = "http://" + consul_address_ + "/v1/agent/check/pass/service:" + service_id;
    std::string response = makeHttpRequest("PUT", url);

    return !response.empty();
}

void ConsulServiceRegistry::watchServices(const std::string& service_name,
                                          std::function<void(const std::vector<ServiceEndpoint>&)> callback) {
    std::lock_guard<std::mutex> lock(watchers_mutex_);
    watchers_[service_name] = callback;
}

void ConsulServiceRegistry::startHealthCheck() {
    if (health_check_running_) {
        return;
    }

    health_check_running_ = true;
    health_check_thread_ = std::thread([this]() {
        healthCheckLoop();
    });
}

void ConsulServiceRegistry::stopHealthCheck() {
    if (health_check_running_) {
        health_check_running_ = false;
        if (health_check_thread_.joinable()) {
            health_check_thread_.join();
        }
    }
}

std::string ConsulServiceRegistry::getServiceId(const ServiceEndpoint& endpoint) const {
    return endpoint.service_name + "-" + endpoint.host + "-" + std::to_string(endpoint.port);
}

void ConsulServiceRegistry::healthCheckLoop() {
    while (health_check_running_) {
        {
            std::lock_guard<std::mutex> locker(service_mutex_);
            for (const auto& pair : registered_services_) {
                updateHeartbeat(pair.first);
            }
        }
        
        std::this_thread::sleep_for(std::chrono::seconds(30));
    }
}

std::string ConsulServiceRegistry::makeHttpRequest(const std::string& method,
                                                   const std::string& url,
                                                   const std::string& body) {
    // 初始化CURL
    CURL* curl = curl_easy_init();
    if (!curl) {
        return "";
    }
    std::string response_data;
    
    // 设置request URL
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    // 设置数据接收回调
    /* contents: 指向接收到的数据块的指针
        size_t size 每个数据元素的大小 通常是一字节
        size_t nmemb 数据元素的数量
        std::string* data  指向response_data的指针（通过CURLOPT_WRITEDDATA设置
        返回实际处理的字节数
    */
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,
        [](void* contents, size_t size, size_t nmemb, std::string* data) {
            data->append((char*)contents, size * nmemb);
            return size * nmemb;
        });
    // 设置存储位置
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_data);
    // 设置超时时间
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
    // 执行HTTP请求
    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        LOG_ERROR("HTTP request failed: " + std::string(curl_easy_strerror(res)));
        return "";
    }
    return response_data;

}

// 解析response

std::vector<ServiceEndpoint> ConsulServiceRegistry::parseServiceList(const std::string& json_response) {
    std::vector<ServiceEndpoint> services;

    Json::Value root;
    Json::Reader reader;

    if (!reader.parse(json_response, root)) {
        LOG_ERROR("Failed to parse service list JSON");
        return services;
    }

    for (const auto& service : root) {
        ServiceEndpoint endpoint;
        endpoint.host = service["Service"]["Address"].asString();
        endpoint.port = service["Service"]["Port"].asInt();
        endpoint.service_name = service["Service"]["Name"].asString();
        endpoint.is_healthy = service["Checks"][0]["Status"].asString() == "passing";

        const auto& meta = service["Service"]["Meta"];
        for (const auto& key : meta.getMemberNames()) {
            endpoint.metadata[key] = meta[key].asString();
        }
        services.push_back(endpoint);
    }

    return services;
}

ServiceEndpoint ConsulServiceRegistry::parseServiceEndpoint(const std::string& json_service) {
    ServiceEndpoint endpoint;
    Json::Value root;
    Json::Reader reader;

    for (const auto& service : root) {
        endpoint.host = service["Service"]["Address"].asString();
        endpoint.port = service["Service"]["Port"].asInt();
        endpoint.service_name = service["Service"]["Name"].asString();
        endpoint.is_healthy = service["Checks"][0]["Status"].asString() == "passing";

        const auto& meta = service["Service"]["Meta"];
        for (const auto& key : meta.getMemberNames()) {
            endpoint.metadata[key] = meta[key].asString();
        }
    }
    return endpoint;
}

// EtcdService
EtcdServiceRegistry::EtcdServiceRegistry() = default;

EtcdServiceRegistry::~EtcdServiceRegistry() {
    if (watch_running_) {
        watch_running_ = false;
        if (watch_thread_.joinable()) {
            watch_thread_.join();
        }
    }
}
bool EtcdServiceRegistry::initialize(const std::string& etcd_address) {
    etcd_address_ = etcd_address;
    LOG_INFO("Etcd service register initialize with address: " + etcd_address);
    return true;
}

bool EtcdServiceRegistry::registerService(const ServiceEndpoint& endpoint) {
    // 简化的etcd注册实现
    std::string service_key = "/services/" + endpoint.service_name + "/" + 
                             endpoint.host + ":" + std::to_string(endpoint.port);
    
    Json::Value service_json;
    service_json["host"] = endpoint.host;
    service_json["port"] = endpoint.port;
    service_json["service_name"] = endpoint.service_name;
    service_json["version"] = endpoint.version;
    service_json["metadata"] = Json::Value(Json::objectValue);
    
    for (const auto& pair : endpoint.metadata) {
        service_json["metadata"][pair.first] = pair.second;
    }
    
    Json::StreamWriterBuilder builder;
    std::string json_string = Json::writeString(builder, service_json);
    
    std::string response = makeEtcdRequest("PUT", service_key, json_string);
    
    {
        std::lock_guard<std::mutex> lock(services_mutex_);
        registered_services_[service_key] = endpoint;
    }
    
    LOG_INFO("Service registered in etcd: " + service_key);
    return !response.empty();
}

bool EtcdServiceRegistry::unregisterService(const std::string& service_id) {
    std::string service_key = "/services/" + service_id;
    std::string response = makeEtcdRequest("DELETE", service_key);
    
    {
        std::lock_guard<std::mutex> lock(services_mutex_);
        registered_services_.erase(service_key);
    }
    
    LOG_INFO("Service unregistered from etcd: " + service_key);
    return !response.empty();
}

std::vector<ServiceEndpoint> EtcdServiceRegistry::discoverServices(const std::string& service_name) {
    std::string service_prefix = "/services/" + service_name + "/";
    std::string response = makeEtcdRequest("GET", service_prefix);
    
    std::vector<ServiceEndpoint> services = parseEtcdResponse(response);
    
    {
        std::lock_guard<std::mutex> lock(services_mutex_);
        discovered_services_[service_name] = services;
    }
    
    return services;
}

bool EtcdServiceRegistry::isServiceHealthy(const std::string& service_id) {
    // 简化的健康检查实现
    return true;
}

bool EtcdServiceRegistry::updateHeartbeat(const std::string& service_id) {
    // 简化的心跳实现
    return true;
}

void EtcdServiceRegistry::watchServices(const std::string& service_name,
                                       std::function<void(const std::vector<ServiceEndpoint>&)> callback) {
    std::lock_guard<std::mutex> lock(watchers_mutex_);
    watchers_[service_name] = callback;
}

void EtcdServiceRegistry::watchLoop() {
    // 简化的监听实现
    while (watch_running_) {
        std::this_thread::sleep_for(std::chrono::seconds(10));
    }
}

std::string EtcdServiceRegistry::makeEtcdRequest(const std::string& method, 
                                                const std::string& key, 
                                                const std::string& value) {
    // 简化的etcd请求实现
    return "OK";
}

std::vector<ServiceEndpoint> EtcdServiceRegistry::parseEtcdResponse(const std::string& response) {
    // 简化的响应解析实现
    return {};
}

// MemoryServiceRegistry 实现
bool MemoryServiceRegistry::registerService(const ServiceEndpoint& endpoint) {
    std::lock_guard<std::mutex> lock(services_mutex_);

    std::string service_id = endpoint.host + ":" + std::to_string(endpoint.port);
    services_[service_id] = endpoint;

    LOG_INFO("Service registered in memory: " + service_id);
    return true;
}

bool MemoryServiceRegistry::unregisterService(const std::string& service_id) {
    std::lock_guard<std::mutex> lock(services_mutex_);
    return services_.erase(service_id) > 0;
}

std::vector<ServiceEndpoint> MemoryServiceRegistry::discoverServices(const std::string& service_name) {
    std::lock_guard<std::mutex> locker(services_mutex_);

    std::vector<ServiceEndpoint> result;
    for (const auto& pair : services_) {
        if (pair.second.service_name == service_name) {
            result.push_back(pair.second);
        }
    }
    return result;
}

bool MemoryServiceRegistry::isServiceHealthy(const std::string& service_id) {
    std::lock_guard<std::mutex> lock(services_mutex_);
    
    auto it = services_.find(service_id);
    return it != services_.end() && it->second.is_healthy;

}

bool MemoryServiceRegistry::updateHeartbeat(const std::string& service_id) {
        std::lock_guard<std::mutex> lock(services_mutex_);
    
    auto it = services_.find(service_id);
    if (it != services_.end()) {
        it->second.last_heartbeat = std::chrono::steady_clock::now();
        return true;
    }
    
    return false;
}

void MemoryServiceRegistry::watchServices(const std::string& service_name,
                                        std::function<void(const std::vector<ServiceEndpoint>&)> callback) {
    std::lock_guard<std::mutex> lock(watchers_mutex_);
    watchers_[service_name] = callback;
}


} // namespace agent_rpc