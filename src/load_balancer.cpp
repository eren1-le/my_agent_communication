/*
 * @Author: eren dengdengd1222@mail.com
 * @Date: 2026-03-16 08:33:39
 * @LastEditors: eren dengdengd1222@mail.com
 * @LastEditTime: 2026-03-16 13:06:29
 * @FilePath: /my_agent_communication/src/load_balancer.cpp
 * @Description: 
 * 
 */
#include "agent_rpc/load_balancer.h"
#include "agent_rpc/logger.h"
namespace agent_rpc {
RoundRobinLoadBalancer::RoundRobinLoadBalancer() = default;
ServiceEndpoint RoundRobinLoadBalancer::selectEndpoint(const std::vector<ServiceEndpoint>& endpoints) {
    if (endpoints.empty()) {
        throw std::runtime_error("No endpoints available");
    }

    std::lock_guard<std::mutex> locker(endpoints_mutex_);

    //过滤健康端点
    std::vector<ServiceEndpoint> healthy_endpoints;
    for (const auto& endpoint : endpoints) {
        if (endpoint.is_healthy) {
            healthy_endpoints.push_back(endpoint);
        }
    }

    if (healthy_endpoints.empty()) {
        throw std::runtime_error("No healthy endpoints available");
    }
    //轮询选择
    size_t index = current_index_.fetch_add(1) % healthy_endpoints_.size();
    return healthy_endpoints[index];
}

void RoundRobinLoadBalancer::updateEndpoints(const std::vector<ServiceEndpoint>& endpoints) {
    std::lock_guard<std::mutex> locker(endpoints_mutex_);
    healthy_endpoints_ = endpoints;
    current_index_ = 0;
}
void RoundRobinLoadBalancer::markEndpointStatus(const std::string& endpoint_id, bool healthy) {
    std::lock_guard<std::mutex> locker(endpoints_mutex_);
    for (auto& endpoint : healthy_endpoints_) {
        if (endpoint.host + ":" + std::to_string(endpoint.port) == endpoint_id) {
            endpoint.is_healthy = healthy;
            break;
        }
    }
}

//RandomLoadBalancer 实现
RandomLoadBalancer::RandomLoadBalancer() : gen_(rd_()) {}

ServiceEndpoint RandomLoadBalancer::selectEndpoint(const std::vector<ServiceEndpoint>& endpoints) {
    if (endpoints.empty()) {
        throw std::runtime_error("No endpoints available");
    }
    std::lock_guard<std::mutex> locker(endpoints_mutex_);
    
    // 过滤健康端点
    std::vector<ServiceEndpoint> healthy_endpoints;

    for (const auto& endpoint : endpoints) {
        if (endpoint.is_healthy) {
            healthy_endpoints.push_back(endpoint);
        }
    }

    if (healthy_endpoints.empty()) {
        throw std::runtime_error("No healthy endpoints available");
    }

    //随机选择
    std::uniform_int_distribution<> dis(0, healthy_endpoints.size() - 1); //创建一个分布器，可以生成[0, healthy_endpoints.size() - 1] 范围内的随机整数
    return healthy_endpoints[dis(gen_)];

}
void RandomLoadBalancer::updateEndpoints(const std::vector<ServiceEndpoint>& endpoints) {
    std::lock_guard<std::mutex> lock(endpoints_mutex_);
    healthy_endpoints_ = endpoints;
}

void RandomLoadBalancer::markEndpointStatus(const std::string& endpoint_id, bool healthy) {
    std::lock_guard<std::mutex> lock(endpoints_mutex_);
    for (auto& endpoint : healthy_endpoints_) {
        if (endpoint.host + ":" + std::to_string(endpoint.port) == endpoint_id) {
            endpoint.is_healthy = healthy;
            break;
        }
    }
}

//最少连接数
LeastConnectionsLoadBalancer::LeastConnectionsLoadBalancer() = default;

ServiceEndpoint LeastConnectionsLoadBalancer::selectEndpoint(const std::vector<ServiceEndpoint>& endpoints) {
    if (endpoints.empty()) {
        throw std::runtime_error("No endpoints to use");
    }
    
    std::lock_guard<std::mutex> locker(endpoints_mutex_);

    ServiceEndpoint* chosen_point = nullptr;
    int min_connection = INT32_MAX;

    for (const auto& endpoint : endpoints) {
        if (!endpoint.is_healthy) continue;
        std::string endpoint_id = endpoint.host + ":" + std::to_string(endpoint.port);
        
        int connection_num = connection_counts_[endpoint_id];

        if (min_connection < connection_num) {
            min_connection = connection_num;
            chosen_point = const_cast<ServiceEndpoint*>(&endpoint);
        }
        
    }
    if (!chosen_point) {
        throw std::runtime_error("No healthy endpoints to use");
    }
    std::string endpoint_id = chosen_point->host + ":"  + std::to_string(chosen_point->port);
    connection_counts_[endpoint_id]++;
    return *chosen_point;
}


void LeastConnectionsLoadBalancer::updateEndpoints(const std::vector<ServiceEndpoint>& endpoints) {
    std::lock_guard<std::mutex> locker(endpoints_mutex_);

    std::set<std::string> current_endpoints;

    for (const auto& endpoint : endpoints) {
        current_endpoints.insert(endpoint.host + ":" + std::to_string(endpoint.port));
    }

    for (auto& it : connection_counts_) {
        if (current_endpoints.find(it.first) == current_endpoints.end()) {
            connection_counts_.erase(it.first);
        }
    }
    
    for (const auto& it : endpoints) {
        endpoints_[it.host + ":" + std::to_string(it.port)] = it;
    }

}
void LeastConnectionsLoadBalancer::markEndpointStatus(const std::string& endpoint_id, bool healthy) {
    std::lock_guard<std::mutex> lock(endpoints_mutex_);
    for (auto& endpoint : endpoints_) {
        if (endpoint.first == endpoint_id) {
            endpoint.second.is_healthy = healthy;
            break;
        }
    }
}

void LeastConnectionsLoadBalancer::decrementConnections(const std::string& endpoint_id) {
    std::lock_guard<std::mutex> locker(endpoints_mutex_);
    auto it = connection_counts_.find(endpoint_id);
    if (it != connection_counts_.end()) {
        it->second--;
    }
    else {
        throw std::runtime_error("Can't find this service");
    }
}
void LeastConnectionsLoadBalancer::incrementConnections(const std::string& endpoint_id) {
    std::lock_guard<std::mutex> locker(endpoints_mutex_);
    auto it = connection_counts_.find(endpoint_id);
    if (it != connection_counts_.end()) {
        it->second++;
    }
    else {
        throw std::runtime_error("Can't find this service");
    } 
}

//加权轮询负载均衡器
WeightedRoundRobinLoadBalancer::WeightedRoundRobinLoadBalancer() = default;

//找出当前最大权重
ServiceEndpoint WeightedRoundRobinLoadBalancer::selectEndpoint(const std::vector<ServiceEndpoint>& endpoints) {
    if (endpoints.empty()) {
        throw std::runtime_error("No endpoints available");
    }

    std::lock_guard<std::mutex> locker(endpoints_mutex_);

    if (weighted_endpoints_.empty()) {
        throw std::runtime_error("No weighted endpoints available");
    }

    WeightEndPoint* best_weight_point = nullptr;
    int max_current_weight = 0;
    //累加权重
    for (auto& endpoint: weighted_endpoints_) {
        if (!endpoint.endpoint.is_healthy) continue;
            endpoint.current_weight += endpoint.weight;
        if (max_current_weight < endpoint.current_weight) {
            max_current_weight = endpoint.current_weight;
            best_weight_point = &endpoint;
        }
    } 
    if (!best_weight_point) {
        throw std::runtime_error("No best weighted endpoints available");
    }
    // 减少使用的节点的权重
    best_weight_point->current_weight -= best_weight_point->weight;
    return best_weight_point->endpoint;

}
void WeightedRoundRobinLoadBalancer::updateEndpoints(const std::vector<ServiceEndpoint>& endpoints) {
    std::lock_guard<std::mutex> locker(endpoints_mutex_);

    weighted_endpoints_.clear();

    for (const auto& endpoint : endpoints) {
        WeightEndPoint weighted_endpoint;
        weighted_endpoint.current_weight = 0;
        weighted_endpoint.endpoint = endpoint;
        weighted_endpoint.weight = 1;   //默认权重

        // 从元数据获得权重
        auto it = endpoint.metadata.find("weight");
         
        if (it != endpoint.metadata.end()) {

                weighted_endpoint.weight = std::stoi(it->second);
 
        }
        weighted_endpoints_.push_back(weighted_endpoint);
    }
}

void WeightedRoundRobinLoadBalancer::markEndpointStatus(const std::string& endpoint_id, bool healthy) {
    std::lock_guard<std::mutex> locker(endpoints_mutex_);

    for (auto& endpoint : weighted_endpoints_) {
        if(endpoint.endpoint.host+ ":" + std::to_string(endpoint.endpoint.port) == endpoint_id) {
            endpoint.endpoint.is_healthy = healthy;
        }
    }
}


} // namespace agent_rpc