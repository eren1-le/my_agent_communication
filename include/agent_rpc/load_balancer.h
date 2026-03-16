/*
 * @Author: eren dengdengd1222@mail.com
 * @Date: 2026-03-16 08:33:13
 * @LastEditors: eren dengdengd1222@mail.com
 * @LastEditTime: 2026-03-16 13:14:11
 * @FilePath: /my_agent_communication/include/agent_rpc/load_balancer.h
 * @Description: 
 * 
 */
#pragma once
#include "rpc_framework.h"
#include <random>
#include <algorithm>
#include <set>
namespace agent_rpc {
//负载均衡策略枚举
enum class LoadBalanceStategy {
    ROUND_ROBIN,            // 轮询
    RANDOM,                 // 随机
    WEIGHTED_ROUND_ROBIN,   // 加权轮询
    LEAST_CONNECTIONS,      // 最少连接
    CONSISTENT_HASH,        // 一致性哈希
    LEAST_RESPONSE_TIME     // 最少响应时间
};

//负载器接口

class LoadBalancer {
public:
    virtual ~LoadBalancer() = default;
    
    //选择服务端点
    virtual ServiceEndpoint selectEndpoint(const std::vector<ServiceEndpoint>& endpoints) = 0;

    //更新服务端点列表
    virtual void updateEndpoints(const std::vector<ServiceEndpoint>& endpoints) = 0;

    //标记服务端点状态
    virtual void markEndpointStatus(const std::string& endpoint_id, bool healthy) = 0;

    //获取策略名称
    virtual std::string getStrategyName() const = 0;

};


//轮询负载均衡器
class RoundRobinLoadBalancer : public LoadBalancer {
public:
    RoundRobinLoadBalancer();
    ~RoundRobinLoadBalancer() = default;

    ServiceEndpoint selectEndpoint(const std::vector<ServiceEndpoint>& endpoints) override;
    void updateEndpoints(const std::vector<ServiceEndpoint>& endpoints) override;
    void markEndpointStatus(const std::string& endpoint_id_, bool healthy) override;

    std::string getStrategyName() const override { return "RoundRobin"; }
private:
    std::atomic<size_t> current_index_{0};
    mutable std::mutex  endpoints_mutex_;
    std::vector<ServiceEndpoint> healthy_endpoints_;
};
//随机负载均衡器
class RandomLoadBalancer : public LoadBalancer {
public:
    RandomLoadBalancer();
    ~RandomLoadBalancer();

    ServiceEndpoint selectEndpoint(const std::vector<ServiceEndpoint>& endpoints) override;
    void updateEndpoints(const std::vector<ServiceEndpoint>& endpoints) override;
    void markEndpointStatus(const std::string& endpoint_id, bool healthy) override;
    std::string getStrategyName() const override { return "Random"; }

private:
    mutable std::mutex endpoints_mutex_;
    std::vector<ServiceEndpoint> healthy_endpoints_;
    std::random_device rd_;
    mutable std::mt19937 gen_;
};

//最少连接负载均衡器
class LeastConnectionsLoadBalancer : public LoadBalancer {
public:
    LeastConnectionsLoadBalancer();
    ~LeastConnectionsLoadBalancer() = default;

    ServiceEndpoint selectEndpoint(const std::vector<ServiceEndpoint>& endpoints) override;
    void updateEndpoints(const std::vector<ServiceEndpoint>& endpoints) override;
    void markEndpointStatus(const std::string& endpoint_id, bool healthy) override;
    std::string getStrategyName() const override { return "LeastConnections"; }


    //连接管理
    void incrementConnections(const std::string& endpoints_id);
    void decrementConnections(const std::string& endpoints_id);


private:
    mutable std::mutex endpoints_mutex_;
    std::map<std::string, ServiceEndpoint> endpoints_;
    std::map<std::string, int> connection_counts_;
};

class WeightedRoundRobinLoadBalancer : public LoadBalancer {
public:
    WeightedRoundRobinLoadBalancer();
    ~WeightedRoundRobinLoadBalancer() = default;

    ServiceEndpoint selectEndpoint(const std::vector<ServiceEndpoint>& endpoints) override;
    void updateEndpoints(const std::vector<ServiceEndpoint>& endpoints) override;
    void markEndpointStatus(const std::string& endpoint_id, bool healthy) override;
    std::string getStrategyName() const override { return "WeightenRoundRobin"; }
private:
struct WeightEndPoint {
    ServiceEndpoint endpoint;
    int weight;
    int current_weight;
};

    mutable std::mutex endpoints_mutex_;
    std::atomic<size_t> current_index_{0};
    std::vector<WeightEndPoint> weighted_endpoints_;
};

// 哈希负载均衡器
class ConsistentHashLoadBalancer : public LoadBalancer {
public:
    ConsistentHashLoadBalancer();
    ~ConsistentHashLoadBalancer() = default;
    
    
    ServiceEndpoint selectEndpoint(const std::vector<ServiceEndpoint>& endpoints) override;
    void updateEndpoints(const std::vector<ServiceEndpoint>& endpoints) override;
    void markEndpointStatus(const std::string& endpoint_id, bool healthy) override;
    std::string getStrategyName() const override { return "Hash"; }

private:


};
} // namespace agent_rpc