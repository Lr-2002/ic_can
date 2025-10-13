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

#include <cstdint>
#include <vector>
#include <string>
#include <functional>
#include <thread>
#include <atomic>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <chrono>
#include <map>

// dm-tools SDK forward declarations
class usb_class;

// Use void* for dm-tools frame to avoid type conflicts
using DmToolsFrame = void*;


namespace ic_can {


// CAN帧结构
struct CANFrame {
    uint32_t can_id;                      // CAN ID
    std::vector<uint8_t> data;            // 数据载荷 (最大64字节)
    bool is_extended_id;                  // 是否为扩展ID
    bool is_fd_frame;                     // 是否为CAN-FD帧
    std::chrono::steady_clock::time_point timestamp; // 时间戳

    CANFrame() : can_id(0), is_extended_id(false), is_fd_frame(false),
                 timestamp(std::chrono::steady_clock::now()) {}

    CANFrame(uint32_t id, const std::vector<uint8_t>& frame_data,
             bool extended = false, bool fd = false)
        : can_id(id), data(frame_data), is_extended_id(extended),
          is_fd_frame(fd), timestamp(std::chrono::steady_clock::now()) {}
};

// USB2CAN适配器异常类
class USB2CANException : public std::runtime_error {
public:
    explicit USB2CANException(const std::string& message)
        : std::runtime_error("USB2CAN Error: " + message) {}
};

// USB2CAN适配器类
class USB2CANAdapter {
public:
    /**
     * @brief 构造函数
     * @param device_sn 设备序列号
     * @param enable_debug 是否启用调试输出
     */
    explicit USB2CANAdapter(const std::string& device_sn, bool enable_debug = false);

    /**
     * @brief 析构函数
     */
    ~USB2CANAdapter();

    // 禁用拷贝构造和赋值
    USB2CANAdapter(const USB2CANAdapter&) = delete;
    USB2CANAdapter& operator=(const USB2CANAdapter&) = delete;

    // ========== 连接管理接口 ==========

    /**
     * @brief 连接到USB2CAN设备
     * @return 连接是否成功
     */
    bool connect();

    /**
     * @brief 断开连接
     * @return 断开是否成功
     */
    bool disconnect();

    /**
     * @brief 检查是否已连接
     * @return 是否已连接
     */
    bool is_connected() const { return connected_; }

    /**
     * @brief 获取设备序列号
     * @return 设备序列号
     */
    const std::string& get_device_sn() const { return device_sn_; }

    // ========== CAN通信接口 ==========

    /**
     * @brief 发送单个CAN帧
     * @param frame CAN帧
     * @return 发送是否成功
     */
    bool send_frame(const CANFrame& frame);

    /**
     * @brief 发送CAN帧（简化接口）
     * @param can_id CAN ID
     * @param data 数据载荷
     * @return 发送是否成功
     */
    bool send_frame(uint32_t can_id, const std::vector<uint8_t>& data);

    /**
     * @brief 接收单个CAN帧
     * @param frame 接收到的CAN帧
     * @param timeout_us 超时时间（微秒）
     * @return 接收是否成功
     */
    bool receive_frame(CANFrame& frame, int timeout_us = 1000);

    /**
     * @brief 接收CAN帧（简化接口）
     * @param can_id 接收到的CAN ID
     * @param data 接收到的数据
     * @param timeout_us 超时时间（微秒）
     * @return 接收是否成功
     */
    bool receive_frame(uint32_t& can_id, std::vector<uint8_t>& data, int timeout_us = 1000);

    // ========== 批量操作接口 ==========

    /**
     * @brief 批量发送CAN帧
     * @param frames CAN帧列表
     * @return 成功发送的帧数
     */
    size_t send_frames_batch(const std::vector<CANFrame>& frames);

    /**
     * @brief 批量接收CAN帧
     * @param frames 接收到的CAN帧列表
     * @param max_frames 最大帧数
     * @param timeout_us 超时时间（微秒）
     * @return 接收到的帧数
     */
    size_t receive_frames_batch(std::vector<CANFrame>& frames,
                               size_t max_frames = 100, int timeout_us = 1000);

    // ========== 异步接口 ==========

    /**
     * @brief 设置帧接收回调函数
     * @param callback 回调函数
     */
    void set_frame_callback(std::function<void(const CANFrame&)> callback);

    /**
     * @brief 启动异步接收线程
     * @return 启动是否成功
     */
    bool start_async_receive();

    /**
     * @brief 停止异步接收线程
     * @return 停止是否成功
     */
    bool stop_async_receive();

    /**
     * @brief 检查异步接收是否正在运行
     * @return 是否正在运行
     */
    bool is_async_receive_running() const { return receive_thread_running_; }

    // ========== 配置接口 ==========

    /**
     * @brief 设置CAN总线参数
     * @param baud_rate 波特率
     * @param fd_data_rate CAN-FD数据段波特率
     * @return 设置是否成功
     */
    bool set_can_parameters(uint32_t baud_rate = 1000000,
                           uint32_t fd_data_rate = 5000000);

    /**
     * @brief 获取当前CAN参数
     * @param baud_rate 输出波特率
     * @param fd_data_rate 输出CAN-FD数据段波特率
     * @return 获取是否成功
     */
    bool get_can_parameters(uint32_t& baud_rate, uint32_t& fd_data_rate) const;

    // ========== 统计接口 ==========

    /**
     * @brief 获取发送统计信息
     * @return 发送帧数
     */
    uint64_t get_sent_frames_count() const { return sent_frames_count_; }

    /**
     * @brief 获取接收统计信息
     * @return 接收帧数
     */
    uint64_t get_received_frames_count() const { return received_frames_count_; }

    /**
     * @brief 获取错误统计信息
     * @return 错误数
     */
    uint64_t get_error_count() const { return error_count_; }

    /**
     * @brief 重置统计信息
     */
    void reset_statistics();

    // ========== 诊断接口 ==========

    /**
     * @brief 获取设备信息
     * @return 设备信息字典
     */
    std::map<std::string, std::string> get_device_info();

    /**
     * @brief 测试连接
     * @return 测试是否通过
     */
    bool test_connection();

private:
    // ========== 私有成员变量 ==========

    std::string device_sn_;                       // 设备序列号
    bool debug_enabled_;                          // 调试输出开关
    std::atomic<bool> connected_;                 // 连接状态

    // Real dm-tools SDK implementation - use void* to avoid header dependencies
    void* dm_usb_class_;                         // dm-tools USB类实例

    // CAN参数
    uint32_t can_baud_rate_;                      // CAN波特率
    uint32_t can_fd_data_rate_;                   // CAN-FD数据段波特率

    // 异步接收
    std::thread receive_thread_;                  // 接收线程
    std::atomic<bool> receive_thread_running_;    // 接收线程运行标志
    std::atomic<bool> stop_receive_thread_;       // 停止接收线程标志

    // 帧缓存
    std::queue<CANFrame> received_frames_queue_;  // 接收帧队列
    mutable std::mutex frames_queue_mutex_;       // 帧队列互斥锁
    std::condition_variable frames_queue_cv_;     // 帧队列条件变量

    // 回调函数
    std::function<void(const CANFrame&)> frame_callback_; // 帧回调函数
    std::mutex callback_mutex_;                   // 回调函数互斥锁

    // 统计信息
    std::atomic<uint64_t> sent_frames_count_{0};     // 发送帧计数
    std::atomic<uint64_t> received_frames_count_{0}; // 接收帧计数
    std::atomic<uint64_t> error_count_{0};           // 错误计数

    // ========== 私有方法 ==========

    /**
     * @brief 初始化dm-tools SDK
     * @return 初始化是否成功
     */
    bool initialize_dm_sdk();

    /**
     * @brief 清理dm-tools SDK资源
     */
    void cleanup_dm_sdk();

    /**
     * @brief 接收线程主函数
     */
    void receive_thread_main();

    /**
     * @brief 处理接收到的帧
     * @param frame 接收到的帧
     */
    void handle_received_frame(const CANFrame& frame);

    /**
     * @brief 处理接收到的dm-tools帧
     * @param dm_frame dm-tools帧结构
     */
    void handle_received_frame(DmToolsFrame dm_frame);

    /**
     * @brief 调试输出
     * @param message 调试信息
     */
    void debug_print(const std::string& message);

    /**
     * @brief 错误输出
     * @param message 错误信息
     */
    void error_print(const std::string& message) const;

    /**
     * @brief 信息输出
     * @param message 信息
     */
    void info_print(const std::string& message);

    /**
     * @brief 增加错误计数
     */
    void increment_error_count();
};

} // namespace ic_can