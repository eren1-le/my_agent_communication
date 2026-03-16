/*
 * @Author: eren dengdengd1222@mail.com
 * @Date: 2026-03-16 08:33:39
 * @LastEditors: eren dengdengd1222@mail.com
 * @LastEditTime: 2026-03-16 20:30:38
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

ConsistentHashLoadBalancer::ConsistentHashLoadBalancer(int virtual_nodes) : virtual_nodes_(virtual_nodes) {}

ServiceEndpoint ConsistentHashLoadBalancer::selectEndpoint(const std::vector<ServiceEndpoint>& endpoints) {
    if (endpoints.empty()) {
        throw std::runtime_error("No endpoints available");
    }

    std::lock_guard<std::mutex> locker(ring_mutex_);

    if (hash_ring_.empty()) {
        throw std::runtime_error("Hash ring is empty");
    }

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 100000);
    std::string key = std::to_string(dis(gen));

    uint32_t hash_value = hash(key);

    return findEndpoint(hash_value);
}
ServiceEndpoint ConsistentHashLoadBalancer::selectEndpointByKey(const std::string& key,const  std::vector<ServiceEndpoint>& ennpoints) {
    if (endpoints_.empty()) {
        throw std::runtime_error("No endpoints available");
    }

    std::lock_guard<std::mutex> locker(ring_mutex_);

    if (hash_ring_.empty()) {
        throw std::runtime_error("Hash ring is empty");
    }

    uint32_t hash_value = hash(key);
    return findEndpoint(hash_value);
}
uint32_t ConsistentHashLoadBalancer::hash(const std::string& key) {
    //简单hash函数
    uint32_t hash = 0;
    for (char c : key) {
        hash = hash * 31 + c;
    }
}

ServiceEndpoint ConsistentHashLoadBalancer::findEndpoint(uint32_t hash_value)
{
    if (hash_ring_.empty()) {
        throw std::runtime_error("Hash ring is empty");
    }

    auto it = std::lower_bound(hash_ring_.begin(), hash_ring_.end(), hash_value,
                               [](const HashNode& node, uint32_t value) {
                                return node.hash < value;
                               });
    if (it == hash_ring_.end()) {
        it = hash_ring_.begin();
    }
    return it->endpoint;
}
void ConsistentHashLoadBalancer::buildHashRing() {
    hash_ring_.clear();

    for (const auto& pair : endpoints_) {
        if (!pair.second.is_healthy) continue;

        //为每个节点创建虚拟节点
        for (int i = 0; i < virtual_nodes_; ++i) {
            std::string virtual_key = pair.first + "#" + std::to_string(i);
            uint32_t hash_value = hash(virtual_key);

            HashNode node;

            node.key = virtual_key;
            node.endpoint = pair.second;
            node.hash = hash_value;

            hash_ring_.push_back(node);
        }
    }

    // 按哈希值排序
    std::sort(hash_ring_.begin(), hash_ring_.end(),
              [](const HashNode& a, const HashNode& b) {
                    return a.hash < b.hash;
              });
}

void ConsistentHashLoadBalancer::updateEndpoints(const std::vector<ServiceEndpoint>& endpoints) {
    std::lock_guard<std::mutex> locker(ring_mutex_);

    endpoints_.clear();

    for (const auto& endpoint : endpoints) {
        endpoints_[endpoint.host + ":" + std::to_string(endpoint.port)] = endpoint;
    }

    buildHashRing();

}

void ConsistentHashLoadBalancer::markEndpointStatus(const std::string& endpoint_id, bool healthy) {
    std::lock_guard<std::mutex> locker(ring_mutex_);

    auto it = endpoints_.find(endpoint_id);
    if (it != endpoints_.end()) {
        it->second.is_healthy = healthy;
        buildHashRing();
    }
}
//最短响应时间
LeastResponseTimeLoadBalancer::LeastResponseTimeLoadBalancer() = default;

ServiceEndpoint LeastResponseTimeLoadBalancer::selectEndpoint(const std::vector<ServiceEndpoint>& endpoints) {
    if (endpoints.empty()) {
        throw std::runtime_error("No endpoints availabel");
    }

    std::lock_guard<std::mutex> locker(stats_mutex_);
    ServiceEndpoint* best_endpoint = nullptr;
    std::chrono::milliseconds min_response_time = std::chrono::milliseconds::max();
    for (const auto& endpoint : endpoints) {
        if (!endpoint.is_healthy) continue;

        std::string endpoint_id = endpoint.host + ":" + std::to_string(endpoint.port);

        auto it = endpoints_stats_.find(endpoint_id);

        if (it != endpoints_stats_.end())  {
            auto response_time = it->second.avg_response_time;
            if (response_time < min_response_time) {
                best_endpoint = const_cast<ServiceEndpoint*>(&endpoint);
                min_response_time = response_time;
            }
        }
        else {
            // 新端点,使用默认响应时间
            if (best_endpoint = nullptr) {
                best_endpoint = const_cast<ServiceEndpoint*>(&endpoint);
            }
        }
    }
    if (!best_endpoint) {
        throw std::runtime_error("No healthy endpoints available");
    }

    return *best_endpoint;
}

void LeastResponseTimeLoadBalancer::updateEndpoints(const std::vector<ServiceEndpoint>& endpoints) {
    std::lock_guard<std::mutex> locker(stats_mutex_);
    // 清理不存在的端点统计
    std::set<std::string> current_endpoints;
    for (const auto& endpoint : endpoints) {
        current_endpoints.insert(endpoint.host + ":" + std::to_string(endpoint.port));
    }

    auto it = endpoints_stats_.begin();
    while (it != endpoints_stats_.end()) {
        if(current_endpoints.find(it->first) == current_endpoints.end()) {
            endpoints_stats_.erase(it);
        } else {
            ++it;
        }
    }
}
void LeastResponseTimeLoadBalancer::markEndpointStatus(const std::string& endpoint_id, bool healthy) {
    std::lock_guard<std::mutex> locker(stats_mutex_);
    auto it = endpoints_stats_.find(endpoint_id);
    if (it != endpoints_stats_.end()) {
        it->second.endpoint.is_healthy = healthy;
    }
}
void LeastResponseTimeLoadBalancer::updateResponseTime(const std::string& endpoint_id, std::chrono::milliseconds response_time) {
    std::lock_guard<std::mutex> locker(stats_mutex_);
    
    auto stats = endpoints_stats_[endpoint_id];
    stats.response_count++;
    stats.last_update = std::chrono::steady_clock::now();

    if (stats.response_count == 1) {
        stats.avg_response_time = response_time;
    }
    else {
        stats.avg_response_time = std::chrono::milliseconds(static_cast<int>(
            (stats.avg_response_time.count() * 0.8) + (response_time.count() * 0.2))
        );
    }
    
}

std::chrono::milliseconds LeastResponseTimeLoadBalancer::calculateAverageResponseTime(const std::string& endpoint_id) {
    std::lock_guard<std::mutex> locker(stats_mutex_);
    auto it = endpoints_stats_.find(endpoint_id);
    if (it == endpoints_stats_.end()) {
        return std::chrono::milliseconds(1000); // 默认为一秒
    }
    return it->second.avg_response_time;
}

// 负载均衡器工厂
std::unique_ptr<LoadBalancer> LoadBalancerFactory::createLoadBalancer(LoadBalanceStategy strategy) {
    switch (strategy)
    {
    case LoadBalanceStategy::RANDOM :
        return std::make_unique<RandomLoadBalancer>();
        break;
    case LoadBalanceStategy::ROUND_ROBIN :
        return std::make_unique<RoundRobinLoadBalancer>();
        break; 
    case LoadBalanceStategy::LEAST_CONNECTIONS :
        return std::make_unique<LeastConnectionsLoadBalancer>();
        break;
    case LoadBalanceStategy::WEIGHTED_ROUND_ROBIN :
        return std::make_unique<WeightedRoundRobinLoadBalancer>();
        break;
    case LoadBalanceStategy::CONSISTENT_HASH :
        return std::make_unique<ConsistentHashLoadBalancer>();
        break;   
    case LoadBalanceStategy::LEAST_RESPONSE_TIME :
        return std::make_unique<LeastResponseTimeLoadBalancer>();
        break;
    default:
        break;
    }
}

std::vector<std::string> LoadBalancerFactory::getAvailableStrategies() {
    return {
        "RoundRobin",
        "Random", 
        "LeastConnections",
        "WeightedRoundRobin",
        "ConsistentHash",
        "LeastResponseTime"
    };
}
} // namespace agent_rpc