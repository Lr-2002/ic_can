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

#include <ic_can/protocols/usb2can_adapter.hpp>
#include <iostream>
#include <chrono>
#include <thread>
#include <cstring>

// 使用真实的dm-tools SDK (Linux Ubuntu)
#define private public
#include "../../dm-tools/USB2FDCAN/SDK/C++/ubuntu/include/protocol/usb_class.h"
#include "../../dm-tools/USB2FDCAN/SDK/C++/ubuntu/include/protocol/damiao.h"
#undef private

namespace ic_can {

USB2CANAdapter::USB2CANAdapter(const std::string& device_sn, bool enable_debug)
    : device_sn_(device_sn), debug_enabled_(enable_debug), connected_(false),
      dm_usb_class_(nullptr), can_baud_rate_(1000000), can_fd_data_rate_(5000000),
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

    try {
        // 创建真实的dm-tools USB类实例
        // 注意：usb_class构造函数会自动连接设备，不需要再调用usb_open()
        dm_usb_class_ = new usb_class(can_baud_rate_, can_fd_data_rate_, device_sn_);
        if (!dm_usb_class_) {
            error_print("Failed to create dm-tools USB class");
            return false;
        }

        // usb_class构造函数已经自动连接了设备
        connected_ = true;
        info_print("Connected to dm-tools USB2CAN device successfully");

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

    try {
        // 清理dm-tools USB类
        if (dm_usb_class_) {
            // Note: usb_class doesn't have explicit disconnect method in destructor
            // Cast to correct type for deletion
            auto* usb_cls = static_cast<class usb_class*>(dm_usb_class_);
            delete usb_cls;
            dm_usb_class_ = nullptr;
        }

        connected_ = false;
        info_print("Disconnected from dm-tools USB2CAN device");
        return true;

    } catch (const std::exception& e) {
        error_print("Disconnect failed: " + std::string(e.what()));
        return false;
    }
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

    try {
        // 使用dm-tools SDK发送CAN帧
        auto* usb_cls = static_cast<class usb_class*>(dm_usb_class_);
        std::vector<uint8_t> data_vector(frame.data.begin(), frame.data.end());
        usb_cls->fdcanFrameSend(data_vector, frame.can_id);
        // 注意：fdcanFrameSend返回void，如果函数执行完没有异常就认为发送成功

        sent_frames_count_++;
        debug_print("Frame sent: ID=0x" + std::to_string(frame.can_id) +
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

    try {
        auto start_time = std::chrono::steady_clock::now();

        while (std::chrono::duration_cast<std::chrono::microseconds>(
               std::chrono::steady_clock::now() - start_time).count() < timeout_us) {

            // dm-tools SDK的接收逻辑会在回调中处理
            // 这里等待接收线程处理完成
            std::this_thread::sleep_for(std::chrono::microseconds(100));

            // 检查接收队列
            {
                std::lock_guard<std::mutex> lock(frames_queue_mutex_);
                if (!received_frames_queue_.empty()) {
                    frame = received_frames_queue_.front();
                    received_frames_queue_.pop();
                    received_frames_count_++;
                    debug_print("Frame received: ID=0x" + std::to_string(frame.can_id));
                    return true;
                }
            }
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
        // 设置dm-tools SDK的回调函数 (基于工作的test_dm_fixed.cpp)
        auto* usb_cls = static_cast<class usb_class*>(dm_usb_class_);
        usb_cls->setFrameCallback([this](can_value_type& can_value) {
            // 直接调用用户回调，避免不必要的转换
            {
                std::lock_guard<std::mutex> lock(callback_mutex_);
                if (frame_callback_) {
                    try {
                        // 转换为IC_CAN格式
                        CANFrame frame;
                        frame.can_id = can_value.head.id;
                        frame.data.resize(can_value.head.dlc);
                        memcpy(frame.data.data(), can_value.data, can_value.head.dlc);
                        frame.timestamp = std::chrono::steady_clock::now();

                        frame_callback_(frame);
                        received_frames_count_++;
                        debug_print("Frame received: ID=0x" + std::to_string(frame.can_id));
                    } catch (const std::exception& e) {
                        error_print("Frame callback error: " + std::string(e.what()));
                        increment_error_count();
                    }
                }
            }
        });

        // 启动dm-tools的数据捕获
        if (usb_cls->USB_CMD_START_CAP() != 0) {
            error_print("Failed to start dm-tools data capture");
            return false;
        }

        stop_receive_thread_ = false;
        receive_thread_running_ = true;

        debug_print("Async receive started with direct callback");
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

    // 停止dm-tools的数据捕获
    if (dm_usb_class_) {
        auto* usb_cls = static_cast<class usb_class*>(dm_usb_class_);
        usb_cls->USB_CMD_STOP_CAP();
    }

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

    // dm-tools的CAN参数在构造时设置，这里只是更新本地变量
    can_baud_rate_ = baud_rate;
    can_fd_data_rate_ = fd_data_rate;

    debug_print("CAN parameters updated");
    return true;
}

// ========== 私有方法实现 ==========

void USB2CANAdapter::receive_thread_main() {
    debug_print("Receive thread started");

    while (!stop_receive_thread_) {
        try {
            // dm-tools SDK会在回调中处理接收，这里只需要监控状态
            std::this_thread::sleep_for(std::chrono::milliseconds(10));

        } catch (const std::exception& e) {
            error_print("Receive thread error: " + std::string(e.what()));
            increment_error_count();
        }
    }

    debug_print("Receive thread stopped");
}

void USB2CANAdapter::handle_received_frame(DmToolsFrame dm_frame) {
    // 将void*转换为dm-tools的can_value_type
    auto* can_value = static_cast<can_value_type*>(dm_frame);
    if (!can_value) return;

    // 将dm-tools的can_value_type转换为我们的CANFrame格式
    CANFrame frame;
    frame.can_id = can_value->head.id;
    frame.data.resize(can_value->head.dlc);
    memcpy(frame.data.data(), can_value->data, can_value->head.dlc);
    frame.timestamp = std::chrono::steady_clock::now();

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

} // namespace ic_can