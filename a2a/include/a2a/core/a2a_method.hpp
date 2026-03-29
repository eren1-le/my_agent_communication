#pragma once

#include <string>

namespace a2a {

/**
 * @brief COnstans for A2A JSON_GRP method names
 */

 class A2AMethods{
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