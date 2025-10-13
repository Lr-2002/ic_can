// Copyright 2025 IC_CAN Team
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include <vector>
#include <string>
#include <atomic>
#include <mutex>
#include <thread>
#include <chrono>
#include <functional>
#include <map>
#include "../motors/base_motor.hpp"

namespace ic_can {

// 安全违规类型
enum class SafetyViolationType {
    POSITION_LIMIT,      // 位置超限
    VELOCITY_LIMIT,      // 速度超限
    TORQUE_LIMIT,        // 力矩超限
    TEMPERATURE_LIMIT,   // 温度超限
    COMMUNICATION_ERROR, // 通信错误
    MOTOR_ERROR,         // 电机错误
    EMERGENCY_STOP,      // 紧急停止
    WATCHDOG_TIMEOUT,    // 看门狗超时
    SOFTWARE_ERROR       // 软件错误
};

// 安全违规信息
struct SafetyViolation {
    SafetyViolationType type;                  // 违规类型
    int motor_id;                              // 电机ID (0表示系统级)
    std::string description;                   // 描述
    std::chrono::steady_clock::time_point timestamp; // 发生时间
    double value;                              // 违规值
    double limit;                              // 限制值
    bool active;                               // 是否仍然活跃

    SafetyViolation(SafetyViolationType t, int id, const std::string& desc,
                   double val, double lim)
        : type(t), motor_id(id), description(desc), value(val), limit(lim),
          timestamp(std::chrono::steady_clock::now()), active(true) {}
};

// 安全限制配置
struct SafetyLimits {
    std::vector<double> position_min;          // 最小位置限制 (弧度)
    std::vector<double> position_max;          // 最大位置限制 (弧度)
    std::vector<double> velocity_max;          // 最大速度限制 (弧度/秒)
    std::vector<double> torque_max;            // 最大力矩限制 (牛顿·米)
    std::vector<double> temperature_max;       // 最大温度限制 (摄氏度)
    double watchdog_timeout_ms;                // 看门狗超时时间 (毫秒)
    double communication_timeout_ms;           // 通信超时时间 (毫秒)

    SafetyLimits() : watchdog_timeout_ms(100.0), communication_timeout_ms(50.0) {}
};

// 安全模块类
class SafetyModule {
public:
    /**
     * @brief 构造函数
     * @param motor_count 电机数量
     */
    explicit SafetyModule(int motor_count = 9);

    /**
     * @brief 析构函数
     */
    ~SafetyModule();

    // 禁用拷贝构造和赋值
    SafetyModule(const SafetyModule&) = delete;
    SafetyModule& operator=(const SafetyModule&) = delete;

    // ========== 配置接口 ==========

    /**
     * @brief 设置安全限制
     * @param limits 安全限制配置
     */
    void set_safety_limits(const SafetyLimits& limits);

    /**
     * @brief 获取安全限制
     * @return 安全限制配置
     */
    SafetyLimits get_safety_limits() const;

    /**
     * @brief 设置位置限制
     * @param motor_id 电机ID
     * @param min_pos 最小位置 (弧度)
     * @param max_pos 最大位置 (弧度)
     */
    void set_position_limits(int motor_id, double min_pos, double max_pos);

    /**
     * @brief 设置速度限制
     * @param motor_id 电机ID
     * @param max_vel 最大速度 (弧度/秒)
     */
    void set_velocity_limit(int motor_id, double max_vel);

    /**
     * @brief 设置力矩限制
     * @param motor_id 电机ID
     * @param max_torque 最大力矩 (牛顿·米)
     */
    void set_torque_limit(int motor_id, double max_torque);

    /**
     * @brief 设置温度限制
     * @param motor_id 电机ID
     * @param max_temp 最大温度 (摄氏度)
     */
    void set_temperature_limit(int motor_id, double max_temp);

    // ========== 安全检查接口 ==========

    /**
     * @brief 检查位置限制
     * @param motor_id 电机ID
     * @param position 位置值
     * @return 是否在限制范围内
     */
    bool check_position_limits(int motor_id, double position);

    /**
     * @brief 检查速度限制
     * @param motor_id 电机ID
     * @param velocity 速度值
     * @return 是否在限制范围内
     */
    bool check_velocity_limits(int motor_id, double velocity);

    /**
     * @brief 检查力矩限制
     * @param motor_id 电机ID
     * @param torque 力矩值
     * @return 是否在限制范围内
     */
    bool check_torque_limits(int motor_id, double torque);

    /**
     * @brief 检查温度限制
     * @param motor_id 电机ID
     * @param temperature 温度值
     * @return 是否在限制范围内
     */
    bool check_temperature_limits(int motor_id, double temperature);

    /**
     * @brief 检查所有安全限制
     * @param motor_id 电机ID
     * @param position 位置值
     * @param velocity 速度值
     * @param torque 力矩值
     * @param temperature 温度值
     * @return 是否全部通过检查
     */
    bool check_all_limits(int motor_id, double position, double velocity,
                         double torque, double temperature);

    // ========== 紧急停止接口 ==========

    /**
     * @brief 触发紧急停止
     * @param reason 停止原因
     * @param motor_id 相关电机ID (0表示系统级)
     */
    void trigger_emergency_stop(const std::string& reason = "Manual emergency stop",
                               int motor_id = 0);

    /**
     * @brief 重置紧急停止状态
     * @return 重置是否成功
     */
    bool reset_emergency_stop();

    /**
     * @brief 检查是否处于紧急停止状态
     * @return 是否处于紧急停止状态
     */
    bool is_emergency_stopped() const { return emergency_stop_triggered_; }

    // ========== 看门狗接口 ==========

    /**
     * @brief 喂狗（重置看门狗计时器）
     */
    void feed_watchdog();

    /**
     * @brief 检查看门狗是否超时
     * @return 是否超时
     */
    bool is_watchdog_timeout() const;

    /**
     * @brief 设置看门狗超时时间
     * @param timeout_ms 超时时间（毫秒）
     */
    void set_watchdog_timeout(double timeout_ms);

    // ========== 通信监控接口 ==========

    /**
     * @brief 报告通信成功
     * @param motor_id 电机ID
     */
    void report_communication_success(int motor_id);

    /**
     * @brief 检查通信超时
     * @param motor_id 电机ID
     * @return 是否超时
     */
    bool is_communication_timeout(int motor_id);

    /**
     * @brief 设置通信超时时间
     * @param timeout_ms 超时时间（毫秒）
     */
    void set_communication_timeout(double timeout_ms);

    // ========== 违规管理接口 ==========

    /**
     * @brief 添加安全违规
     * @param violation 违规信息
     */
    void add_violation(const SafetyViolation& violation);

    /**
     * @brief 清除指定类型的违规
     * @param type 违规类型
     * @param motor_id 电机ID
     */
    void clear_violations(SafetyViolationType type, int motor_id = -1);

    /**
     * @brief 清除所有违规
     */
    void clear_all_violations();

    /**
     * @brief 获取活跃的违规列表
     * @return 活跃违规列表
     */
    std::vector<SafetyViolation> get_active_violations() const;

    /**
     * @brief 获取指定电机的违规列表
     * @param motor_id 电机ID
     * @return 违规列表
     */
    std::vector<SafetyViolation> get_motor_violations(int motor_id) const;

    /**
     * @brief 检查是否有活跃违规
     * @return 是否有活跃违规
     */
    bool has_active_violations() const;

    // ========== 回调接口 ==========

    /**
     * @brief 设置紧急停止回调函数
     * @param callback 回调函数
     */
    void set_emergency_stop_callback(std::function<void(const std::string&)> callback);

    /**
     * @brief 设置违规回调函数
     * @param callback 回调函数
     */
    void set_violation_callback(std::function<void(const SafetyViolation&)> callback);

    // ========== 监控接口 ==========

    /**
     * @brief 启动安全监控线程
     * @return 启动是否成功
     */
    bool start_monitoring();

    /**
     * @brief 停止安全监控线程
     * @return 停止是否成功
     */
    bool stop_monitoring();

    /**
     * @brief 检查监控是否正在运行
     * @return 是否正在运行
     */
    bool is_monitoring_active() const { return monitoring_active_; }

    // ========== 诊断接口 ==========

    /**
     * @brief 获取安全状态摘要
     * @return 状态摘要字典
     */
    std::map<std::string, std::string> get_safety_status_summary();

    /**
     * @brief 打印安全状态报告
     */
    void print_safety_status_report();

    /**
     * @brief 获取安全统计信息
     * @return 统计信息字典
     */
    std::map<std::string, uint64_t> get_safety_statistics();

private:
    // ========== 私有成员变量 ==========

    int motor_count_;                          // 电机数量
    SafetyLimits limits_;                      // 安全限制配置

    // 紧急停止状态
    std::atomic<bool> emergency_stop_triggered_{false}; // 紧急停止标志
    std::string emergency_stop_reason_;        // 紧急停止原因
    std::chrono::steady_clock::time_point emergency_stop_time_; // 紧急停止时间

    // 看门狗
    std::chrono::steady_clock::time_point last_watchdog_feed_; // 上次喂狗时间
    mutable std::mutex watchdog_mutex_;                          // 看门狗互斥锁
    std::atomic<double> watchdog_timeout_ms_{100.0}; // 看门狗超时时间

    // 通信监控
    std::vector<std::chrono::steady_clock::time_point> last_communication_; // 上次通信时间
    std::atomic<double> communication_timeout_ms_{50.0}; // 通信超时时间

    // 违规管理
    std::vector<SafetyViolation> violations_;  // 违规列表
    mutable std::mutex violations_mutex_;      // 违规列表互斥锁

    // 监控线程
    std::thread monitoring_thread_;            // 监控线程
    std::atomic<bool> monitoring_active_{false}; // 监控活动标志
    std::atomic<bool> stop_monitoring_{false};  // 停止监控标志

    // 回调函数
    std::function<void(const std::string&)> emergency_stop_callback_; // 紧急停止回调
    std::function<void(const SafetyViolation&)> violation_callback_; // 违规回调
    std::mutex callback_mutex_;               // 回调函数互斥锁

    // 统计信息
    std::atomic<uint64_t> total_violations_{0};     // 总违规数
    std::atomic<uint64_t> emergency_stops_{0};      // 紧急停止次数
    std::atomic<uint64_t> watchdog_timeouts_{0};    // 看门狗超时次数
    std::atomic<uint64_t> communication_timeouts_{0}; // 通信超时次数

    // ========== 私有方法 ==========

    /**
     * @brief 监控线程主函数
     */
    void monitoring_thread_main();

    /**
     * @brief 执行安全检查
     */
    void perform_safety_checks();

    /**
     * @brief 检查看门狗超时
     */
    void check_watchdog_timeout();

    /**
     * @brief 检查通信超时
     */
    void check_communication_timeouts();

    /**
     * @brief 清理过期的违规记录
     */
    void cleanup_expired_violations();

    /**
     * @brief 触发回调函数
     * @param violation 违规信息
     */
    void trigger_violation_callback(const SafetyViolation& violation);

    /**
     * @brief 触发紧急停止回调
     * @param reason 停止原因
     */
    void trigger_emergency_stop_callback(const std::string& reason);

    /**
     * @brief 违规类型转字符串
     * @param type 违规类型
     * @return 字符串描述
     */
    std::string violation_type_to_string(SafetyViolationType type) const;

    /**
     * @brief 检查电机ID是否有效
     * @param motor_id 电机ID
     * @return 是否有效
     */
    bool is_valid_motor_id(int motor_id) const;

    /**
     * @brief 获取当前时间戳
     * @return 当前时间戳
     */
    std::chrono::steady_clock::time_point get_current_time() const;
};

} // namespace ic_can