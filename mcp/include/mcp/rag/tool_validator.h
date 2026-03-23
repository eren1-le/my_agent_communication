/*
 * @Author: eren dengdengd1222@mail.com
 * @Date: 2026-03-23 20:49:50
 * @LastEditors: eren dengdengd1222@mail.com
 * @LastEditTime: 2026-03-23 21:04:06
 * @FilePath: /my_agent_communication/mcp/include/mcp/rag/tool_validator.h
 * @Description: 
 * 
 */
/**
 * @file tool_validator.h
 * @brief 可选工具验证器
 * 
 */

 #pragma

#include "tool_retriever.h"
#include "../mcp_agent_integration.h"

#include <string>
#include <vector>
#include <functional>
#include <chrono>

namespace agent_rpc {
namespace mcp {
namespace rag {
/**
 * @brief
 */
struct ValidationResult {
    std::string tool_name;            ///< 工具名称
    bool is_valid = false;            ///< 是否有效
    std::string error_message;        ///< 错误信息
    int64_t duration_ms = 0;          ///< 验证耗时   
};

/**
 * @brief 验证器配置
 */
struct ValidatorConfig {
    int timeout_ms = 5000;            ///< 验证超时 (毫秒)
    bool treat_timeout_as_valid = true; ///< 超时时是否视为有效
    int max_test_queries = 3;         ///< 每个工具的最大测试查询数
};
/**
 * @brief 工具调用结果
 */
struct ToolCallResult {
    bool success = false;                  ///< 是否成功
    std::string result;                    ///< 成功时的结果
    std::string error;                     ///< 失败时的错误信息
    int64_t duration_ms = 0;               ///< 调用耗时 (毫秒)
};
/**
 * @brief 工具验证器
 * 
 * 验证检索到的工具是否兼容当前任务。
 * 通过生成测试查询并检查响应来验证工具。
 */

class ToolValidator {
public:
    /**
     * @brief 工具调用函数类型
     * @param tool_name 工具名称
     * @param arguments 参数 JSON
     * @return 调用结果
     */
    using ToolCallFunc = std::function<ToolCallResult(
        const std::string& tool_name, 
        const std::string& arguments)>;
    
        explicit ToolValidator(const ValidatorConfig& config);
        ~ToolValidator() = default;

    /**
     * @brief 设置工具调用函数
     */
    void setToolCallFunc(ToolCallFunc func) { tool_call_func_ = func; }

    /**
     * @brief 验证单个工具
     * @param tool 工具信息
     * @return 验证结果
     */
    ValidationResult validate(const RetrievedTool& tool);
    std::vector<ValidationResult> validateBatch(const std::vector<RetrievedTool>& tools);
    
    /**
     * @brief 过滤无效工具
     * @param tools 工具列表
     * @return 有效工具列表
     */
    std::vector<RetrievedTool> filterInvalid(const std::vector<RetrievedTool>& tools);
    
    /**
     * @brief 获取配置
     */
    const ValidatorConfig& getConfig() const { return config_; }

private:

    /**
     * @brief 生成测试查询
     */
    std::vector<std::string> generateTestQueries(const RetrievedTool& tool);
    
    /**
     * @brief 执行测试查询
     */
    bool executeTestQuery(const std::string& tool_name, const std::string& query);

    ValidatorConfig config_;
    ToolCallFunc tool_call_func_;
};
} // namespace rag
} // namespace mcp
} // namespace agent_rpc