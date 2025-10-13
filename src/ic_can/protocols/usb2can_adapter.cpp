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

#include "ic_can/protocols/usb2can_adapter.hpp"
#include <iostream>
#include <chrono>
#include <thread>
#include <cstring>

// ⚠️ 注意：这里使用前向声明避免直接包含 dm-tools 头文件
// 实际部署时需要正确链接 dm-tools SDK

namespace ic_can {

// dm-tools 前向声明 (模拟)
namespace damiao {
    class Motor_Control;
    class usb_class;
}

USB2CANAdapter::USB2CANAdapter(const std::string& device_sn, bool enable_debug)
    : device_sn_(device_sn), debug_enabled_(enable_debug), connected_(false),
      dm_motor_control_(nullptr), dm_usb_class_(nullptr),
      can_baud_rate_(1000000), can_fd_data_rate_(5000000),
      receive_thread_running_(false), stop_receive_thread_(false),
      sent_frames_count_(0), received_frames_count_(0), error_count_(0) {

    debug_print("USB2CANAdapter created with device SN: " + device_sn_);
}

USB2CANAdapter::~USB2CANAdapter() {
    if (is_connected()) {
        disconnect();
    }
    stop_async_receive();
}

bool USB2CANAdapter::connect() {
    if (connected_) {
        debug_print("Already connected");
        return true;
    }

    debug_print("Attempting to connect to device: " + device_sn_);

    // ⚠️ 仿真：模拟连接过程
    // 实际实现需要：
    // 1. 初始化 damiao::usb_class
    // 2. 设置设备序列号
    // 3. 初始化 damiao::Motor_Control
    // 4. 配置 CAN 参数

    try {
        // 仿真连接成功
        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // 模拟连接延迟

        dm_usb_class_ = reinterpret_cast<void*>(0x12345678); // 仿真指针
        dm_motor_control_ = reinterpret_cast<void*>(0x87654321); // 仿真指针

        connected_ = true;
        debug_print("⚠️ SIMULATED connection successful");

        // 设置 CAN 参数
        if (!set_can_parameters(can_baud_rate_, can_fd_data_rate_)) {
            debug_print("Failed to set CAN parameters");
            disconnect();
            return false;
        }

        info_print("Connected to USB2CAN device successfully");
        return true;

    } catch (const std::exception& e) {
        error_print("Connection failed: " + std::string(e.what()));
        return false;
    }
}

bool USB2CANAdapter::disconnect() {
    if (!connected_) {
        debug_print("Not connected");
        return true;
    }

    debug_print("Disconnecting from device");

    // 停止异步接收
    stop_async_receive();

    // ⚠️ 仿真：模拟断开连接
    // 实际实现需要：
    // 1. 停止数据接收
    // 2. 清理 damiao::Motor_Control
    // 3. 清理 damiao::usb_class

    dm_motor_control_ = nullptr;
    dm_usb_class_ = nullptr;
    connected_ = false;

    info_print("Disconnected from USB2CAN device");
    return true;
}

bool USB2CANAdapter::send_frame(const CANFrame& frame) {
    if (!connected_) {
        error_print("Not connected");
        return false;
    }

    if (frame.data.size() > 64) {
        error_print("Frame data too large: " + std::to_string(frame.data.size()));
        return false;
    }

    // ⚠️ 仿真：模拟发送过程
    try {
        // 实际实现需要调用 dm-tools SDK 发送函数
        // 例如：dm_motor_control_->send_data(can_id, data.data(), data.size());

        // 仿真发送延迟
        std::this_thread::sleep_for(std::chrono::microseconds(50));

        sent_frames_count_++;
        debug_print("⚠️ SIMULATED frame sent: ID=0x" + std::to_string(frame.can_id) +
                    ", Size=" + std::to_string(frame.data.size()));
        return true;

    } catch (const std::exception& e) {
        error_print("Send frame failed: " + std::string(e.what()));
        increment_error_count();
        return false;
    }
}

bool USB2CANAdapter::send_frame(uint32_t can_id, const std::vector<uint8_t>& data) {
    CANFrame frame(can_id, data);
    return send_frame(frame);
}

bool USB2CANAdapter::receive_frame(CANFrame& frame, int timeout_us) {
    if (!connected_) {
        error_print("Not connected");
        return false;
    }

    // ⚠️ 仿真：模拟接收过程
    try {
        auto start_time = std::chrono::steady_clock::now();

        // 仿真接收延迟和随机数据
        while (std::chrono::duration_cast<std::chrono::microseconds>(
               std::chrono::steady_clock::now() - start_time).count() < timeout_us) {

            // 检查接收队列
            std::lock_guard<std::mutex> lock(frames_queue_mutex_);
            if (!received_frames_queue_.empty()) {
                frame = received_frames_queue_.front();
                received_frames_queue_.pop();
                received_frames_count_++;
                debug_print("⚠️ SIMULATED frame received: ID=0x" + std::to_string(frame.can_id));
                return true;
            }

            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }

        return false; // 超时

    } catch (const std::exception& e) {
        error_print("Receive frame failed: " + std::string(e.what()));
        increment_error_count();
        return false;
    }
}

bool USB2CANAdapter::receive_frame(uint32_t& can_id, std::vector<uint8_t>& data, int timeout_us) {
    CANFrame frame;
    if (receive_frame(frame, timeout_us)) {
        can_id = frame.can_id;
        data = frame.data;
        return true;
    }
    return false;
}

size_t USB2CANAdapter::send_frames_batch(const std::vector<CANFrame>& frames) {
    if (!connected_) {
        error_print("Not connected");
        return 0;
    }

    size_t success_count = 0;
    for (const auto& frame : frames) {
        if (send_frame(frame)) {
            success_count++;
        }
    }

    debug_print("Batch send completed: " + std::to_string(success_count) + "/" +
                std::to_string(frames.size()) + " frames sent");
    return success_count;
}

size_t USB2CANAdapter::receive_frames_batch(std::vector<CANFrame>& frames,
                                            size_t max_frames, int timeout_us) {
    if (!connected_) {
        error_print("Not connected");
        return 0;
    }

    frames.clear();
    size_t received_count = 0;

    auto start_time = std::chrono::steady_clock::now();

    while (received_count < max_frames &&
           std::chrono::duration_cast<std::chrono::microseconds>(
               std::chrono::steady_clock::now() - start_time).count() < timeout_us) {

        CANFrame frame;
        if (receive_frame(frame, 1000)) { // 1ms 内部超时
            frames.push_back(frame);
            received_count++;
        }
    }

    debug_print("Batch receive completed: " + std::to_string(received_count) + " frames received");
    return received_count;
}

void USB2CANAdapter::set_frame_callback(std::function<void(const CANFrame&)> callback) {
    std::lock_guard<std::mutex> lock(callback_mutex_);
    frame_callback_ = callback;
    debug_print("Frame callback set");
}

bool USB2CANAdapter::start_async_receive() {
    if (receive_thread_running_) {
        debug_print("Async receive already running");
        return true;
    }

    if (!connected_) {
        error_print("Cannot start async receive: not connected");
        return false;
    }

    try {
        stop_receive_thread_ = false;
        receive_thread_ = std::thread(&USB2CANAdapter::receive_thread_main, this);
        receive_thread_running_ = true;

        debug_print("Async receive thread started");
        return true;

    } catch (const std::exception& e) {
        error_print("Failed to start async receive: " + std::string(e.what()));
        return false;
    }
}

bool USB2CANAdapter::stop_async_receive() {
    if (!receive_thread_running_) {
        debug_print("Async receive not running");
        return true;
    }

    debug_print("Stopping async receive thread");

    stop_receive_thread_ = true;

    if (receive_thread_.joinable()) {
        receive_thread_.join();
    }

    receive_thread_running_ = false;
    debug_print("Async receive thread stopped");
    return true;
}

bool USB2CANAdapter::set_can_parameters(uint32_t baud_rate, uint32_t fd_data_rate) {
    if (!connected_) {
        error_print("Cannot set CAN parameters: not connected");
        return false;
    }

    debug_print("Setting CAN parameters: baud=" + std::to_string(baud_rate) +
                ", fd_data=" + std::to_string(fd_data_rate));

    // ⚠️ 仿真：模拟参数设置
    // 实际实现需要调用 dm-tools SDK 配置函数
    // 例如：dm_motor_control_->setup_can(baud_rate, fd_data_rate);

    can_baud_rate_ = baud_rate;
    can_fd_data_rate_ = fd_data_rate;

    // 仿真设置延迟
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    debug_print("⚠️ SIMULATED CAN parameters set successfully");
    return true;
}

bool USB2CANAdapter::get_can_parameters(uint32_t& baud_rate, uint32_t& fd_data_rate) const {
    if (!connected_) {
        error_print("Cannot get CAN parameters: not connected");
        return false;
    }

    baud_rate = can_baud_rate_;
    fd_data_rate = can_fd_data_rate_;
    return true;
}

void USB2CANAdapter::reset_statistics() {
    sent_frames_count_ = 0;
    received_frames_count_ = 0;
    error_count_ = 0;
    debug_print("Statistics reset");
}

std::map<std::string, std::string> USB2CANAdapter::get_device_info() {
    std::map<std::string, std::string> info;

    if (connected_) {
        info["device_sn"] = device_sn_;
        info["connection_status"] = "connected";
        info["can_baud_rate"] = std::to_string(can_baud_rate_);
        info["can_fd_data_rate"] = std::to_string(can_fd_data_rate_);
        info["sent_frames"] = std::to_string(sent_frames_count_);
        info["received_frames"] = std::to_string(received_frames_count_);
        info["errors"] = std::to_string(error_count_);
    } else {
        info["connection_status"] = "disconnected";
    }

    return info;
}

bool USB2CANAdapter::test_connection() {
    if (!connected_) {
        debug_print("Cannot test connection: not connected");
        return false;
    }

    debug_print("Testing connection");

    // ⚠️ 仿真：发送测试帧并检查响应
    CANFrame test_frame(0x123, {0xAA, 0xBB, 0xCC, 0xDD});

    if (send_frame(test_frame)) {
        // 等待一小段时间
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        debug_print("⚠️ SIMULATED connection test passed");
        return true;
    }

    error_print("Connection test failed");
    return false;
}

// ========== 私有方法实现 ==========

bool USB2CANAdapter::initialize_dm_sdk() {
    // ⚠️ 仿真：模拟 dm-tools SDK 初始化
    // 实际实现需要：
    // 1. 创建 damiao::usb_class 实例
    // 2. 创建 damiao::Motor_Control 实例
    // 3. 配置设备和 CAN 参数

    debug_print("⚠️ SIMULATED dm-tools SDK initialization");
    return true;
}

void USB2CANAdapter::cleanup_dm_sdk() {
    // ⚠️ 仿真：模拟 dm-tools SDK 清理
    // 实际实现需要：
    // 1. 停止数据接收
    // 2. 销毁 damiao::Motor_Control 实例
    // 3. 销毁 damiao::usb_class 实例

    debug_print("⚠️ SIMULATED dm-tools SDK cleanup");
}

void USB2CANAdapter::receive_thread_main() {
    debug_print("Receive thread started");

    while (!stop_receive_thread_) {
        try {
            // ⚠️ 仿真：模拟数据接收
            // 实际实现需要调用 dm-tools SDK 接收函数

            // 生成一些仿真数据
            static uint32_t simulation_counter = 0;
            if (simulation_counter % 100 == 0) { // 每100次循环生成一个帧
                CANFrame simulated_frame;
                simulated_frame.can_id = 0x100 + (simulation_counter % 10);
                simulated_frame.data = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
                simulated_frame.timestamp = std::chrono::steady_clock::now();

                handle_received_frame(simulated_frame);
            }

            simulation_counter++;

            // 控制接收频率
            std::this_thread::sleep_for(std::chrono::microseconds(1000));

        } catch (const std::exception& e) {
            error_print("Receive thread error: " + std::string(e.what()));
            increment_error_count();
        }
    }

    debug_print("Receive thread stopped");
}

void USB2CANAdapter::handle_received_frame(const CANFrame& frame) {
    // 添加到接收队列
    {
        std::lock_guard<std::mutex> lock(frames_queue_mutex_);
        received_frames_queue_.push(frame);

        // 限制队列大小，避免内存溢出
        while (received_frames_queue_.size() > 1000) {
            received_frames_queue_.pop();
        }
    }

    // 调用回调函数
    {
        std::lock_guard<std::mutex> lock(callback_mutex_);
        if (frame_callback_) {
            try {
                frame_callback_(frame);
            } catch (const std::exception& e) {
                error_print("Frame callback error: " + std::string(e.what()));
                increment_error_count();
            }
        }
    }
}

CANFrame USB2CANAdapter::convert_dm_frame_to_can_frame(void* dm_frame) {
    // ⚠️ 仿真：模拟 dm-tools 帧转换
    CANFrame frame;
    frame.can_id = 0x123;
    frame.data = {0x01, 0x02, 0x03, 0x04};
    return frame;
}

bool USB2CANAdapter::convert_can_frame_to_dm_frame(const CANFrame& can_frame, void* dm_frame) {
    // ⚠️ 仿真：模拟 CAN 帧转换
    (void)can_frame;
    (void)dm_frame;
    return true;
}

void USB2CANAdapter::debug_print(const std::string& message) {
    if (debug_enabled_) {
        auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();

        std::cout << "[" << timestamp << "] [USB2CAN-DEBUG] " << message << std::endl;
    }
}

void USB2CANAdapter::error_print(const std::string& message) const {
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();

    std::cerr << "[" << timestamp << "] [USB2CAN-ERROR] " << message << std::endl;
}

void USB2CANAdapter::info_print(const std::string& message) {
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();

    std::cout << "[" << timestamp << "] [USB2CAN-INFO] " << message << std::endl;
}

void USB2CANAdapter::increment_error_count() {
    error_count_++;
}

bool USB2CANAdapter::is_dm_sdk_available() const {
    // ⚠️ 仿真：检查 dm-tools SDK 是否可用
    return true;
}

} // namespace ic_can