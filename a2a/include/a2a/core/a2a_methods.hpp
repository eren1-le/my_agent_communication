/*
 * @Author: eren dengdengd1222@mail.com
 * @Date: 2026-03-29 11:32:14
 * @LastEditors: eren dengdengd1222@mail.com
 * @LastEditTime: 2026-03-31 00:56:09
 * @FilePath: /my_agent_communication/a2a/include/a2a/core/a2a_methods.hpp
 * @Description: 
 * 
 */
#pragma once

#include <string>

namespace a2a {

/**
 * @brief COnstans for A2A JSON_GRP method names
 */

 class A2AMethods{
public:
    // Message methods
    static constexpr const char* MESSAGE_SEND = "message/send";
    static constexpr const char* MESSAGE_STREAM = "message/stream";

    // Task methods
    static constexpr const char* TASK_GET = "tasks/get";
    static constexpr const char* TASK_CANCEL = "tasks/cancle";
    static constexpr const char* TASK_SUBSCRIBE = "tasks/resubscribe";


    // Push notification methods
    static constexpr const char* TASK_PUSH_NOTIFICATION_CONFIG_SET = 
        "tasks/pushNotificationConfig/set";
    static constexpr const char* TASK_PUSH_NOTIFICATION_CONFIG_GET = 
        "tasks/pushNotificationConfig/get";

    /**
     * @brief Check if a method requires streaming response
     */

     static bool is_stream_method(const std::string& method) {
        return method == MESSAGE_STREAM || method == TASK_SUBSCRIBE;
     }


     /**
     * @brief Check if a method name is valid
     */
    static bool is_valid_method(const std::string& method) {
        return method == MESSAGE_SEND ||
               method == MESSAGE_STREAM ||
               method == TASK_GET ||
               method == TASK_CANCEL ||
               method == TASK_SUBSCRIBE ||
               method == TASK_PUSH_NOTIFICATION_CONFIG_SET ||
               method == TASK_PUSH_NOTIFICATION_CONFIG_GET;
    }
    
 };
} // namespace a2a