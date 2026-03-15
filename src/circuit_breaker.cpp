#include "circuit_breaker.h"

namespace agent_rpc {
// CircuitBreaker 实现
    CircuitBreaker::CircuitBreaker(const CircuitBreakerConfig& config) :
    state_(CircuitState::CLOSED),
    config_(config),
    last_state_change_(std::chrono::steady_clock::now()) {}

    template<typename Func>
    auto CircuitBreaker::execute(Func&& func) -> decltype(func()){
        if (!isRequestAllowed()) {
            {
                std::lock_guard<std::mutex> locker(stats_mutex_);
                stats_.rejected_requests++;
            }
            throw std::runtime_error("Circuit breaker is open");
        }
        try {
            auto result = func();
            recordSuccess();
        }
        catch(...) {
            recordFailure();
        }
    }
    void CircuitBreaker::recordSuccess() {
        std::lock_guard<std::mutex> locker(stats_mutex_);
        stats_.successful_requests++;
        stats_.total_requests++;

        updateFailureRate();
        stats_.last_success_time = std::chrono::steady_clock::now();
        if (state_ == CircuitState::HALF_OPRN &&
            stats_.successful_requests >= config_.success_threshold) {
                transitionToClosed();
        }
    }

    void CircuitBreaker::updateFailureRate() {
        if (stats_.total_requests >= config_.min_request_count ) {
            stats_.current_failure_rate = static_cast<double> (stats_.failed_reqeusts / stats_.total_requests);
            if (stats_.current_failure_rate >= config_.failure_rate_threshold) {
                transitionToOpen();
            }
        }
    }

    bool CircuitBreaker::isRequestAllowed() {
        switch (state_)
        {
        case CircuitState::CLOSED:
            return true;
            break;
        case CircuitState::HALF_OPRN:
            return stats_.total_requests < config_.success_threshold;
            break;
        case CircuitState::OPEN:
            if (shouldAttemptReset()) {
                transitionToHalfOpen();
                return true;
            }
            return false;
            break;
        default:
            break;
        }
    }

    void CircuitBreaker::recordFailure() {
        std::lock_guard<std::mutex> locker(stats_mutex_);
        stats_.failed_reqeusts++;
        stats_.total_requests++;
        updateFailureRate();

    }
    bool CircuitBreaker::shouldAttemptReset() {
        auto now = std::chrono::steady_clock::now();
        auto time_since_open = now - last_state_change_;

        return time_since_open >= config_.timeout;
    }
    void CircuitBreaker::transitionToClosed() {
        if (state_ != CircuitState::CLOSED) {
            std::lock_guard<std::mutex> locker(stats_mutex_);
            state_ = CircuitState::CLOSED;
            last_state_change_ = std::chrono::steady_clock::now();
            
            //重置统计信息
            stats_ = CircuitBreakerStates{};

            LOG_INFO("Circuit breaker closed - service recovered");
        }
    }
    void CircuitBreaker::transitionToHalfOpen() {
        if (state_ != CircuitState::HALF_OPRN) {
            std::lock_guard<std::mutex> locker(stats_mutex_);
            state_ = CircuitState::HALF_OPRN;
            last_state_change_ = std::chrono::steady_clock::now();

            //重置成功和失败次数
            stats_.failed_reqeusts = 0;
            stats_.successful_requests = 0;

            LOG_INFO("Circuit breaker half open - service recovered");
        }
    }
    void CircuitBreaker::transitionToOpen() {
        if (state_ != CircuitState::OPEN) {
            std::lock_guard<std::mutex> locker(stats_mutex_);
            state_ = CircuitState::OPEN;
            last_state_change_ = std::chrono::steady_clock::now();
            LOG_WARN("Circuit breaker open - service recovered");
        }
    }

    void CircuitBreaker::reset() {
        std::lock_guard<std::mutex> locker(stats_mutex_);
        stats_ = CircuitBreakerStates{};
        state_ = CircuitState::CLOSED;
        last_state_change_ = std::chrono::steady_clock::now();
    }
    void CircuitBreaker::updateConfig( const CircuitBreakerConfig& config) {
        config_ = config;
    }

//CircuitBreakerManager 实现
CircuitBreakerManager& CircuitBreakerManager::getInstance() {
    static CircuitBreakerManager instance;
    return instance;
}

std::shared_ptr<CircuitBreaker> CircuitBreakerManager::getCircuitBreaker(const std::string& service_name) {
    std::lock_guard<std::mutex> locker(circuit_breakers_mutex_);

    auto it = circuit_breakers_.find(service_name);
    if (it != circuit_breakers_.end()) {
        return it->second;
    }

    //创建新的熔断器
    auto circuit_breaker = std::make_shared<CircuitBreaker>();
    circuit_breakers_[service_name] = circuit_breaker;

    LOG_INFO("Create circuit breaker for service: " + service_name);
    return circuit_breaker;
}

void CircuitBreakerManager::removeCircuitBreaker(const std::string& service_name) {
    std::lock_guard<std::mutex> locker(circuit_breakers_mutex_);

    auto it = circuit_breakers_.find(service_name);

    if (it !=circuit_breakers_.end()) {
        circuit_breakers_.erase(it);
        LOG_INFO("Removed circuit breaker for service: " + service_name);
    }

}

std::map<std::string, CircuitState> CircuitBreakerManager::getAllStates() const {

    std::lock_guard<std::mutex> locker(circuit_breakers_mutex_);
    std::map<std::string, CircuitState> states;

    for(const  auto& it : circuit_breakers_) {
        states[it.first] = it.second->getState();
    }
    return states;
}

void CircuitBreakerManager::resetAll() {
    std::lock_guard<std::mutex> locker(circuit_breakers_mutex_);

    for(auto& it : circuit_breakers_) {
        it.second->reset();
    }

    LOG_INFO("Reset all circuit breakers");
}

void CircuitBreakerManager::updateConfig(const std::string& service_name, const CircuitBreakerConfig& config) {
    std::lock_guard<std::mutex> locker(circuit_breakers_mutex_);
     auto it = circuit_breakers_.find(service_name);

     if(it != circuit_breakers_.end()) {
        it->second->updateConfig(config);
        LOG_INFO("Update circuit breaker config for service: " + service_name);
     }
}

} // namespace agnet_rpc