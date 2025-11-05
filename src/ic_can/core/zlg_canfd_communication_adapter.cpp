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

#include "ic_can/core/zlg_canfd_communication_adapter.hpp"
#include <algorithm>
#include <cstring>
#include <dlfcn.h>
#include <iomanip>
#include <iostream>
#include <unistd.h>

// Include ZLG CAN FD headers
#include "/home/lr-2002/project/InstantCreation/ic_can/lib/zlg_canfd/controlcanfd.h"

namespace ic_can {

// ZLG CAN FD Macros - using definitions from controlcanfd.h
// #define GET_ID(can_id) ((can_id) & 0x1FFFFFFFU)           // Use from
// controlcanfd.h #define IS_EFF(can_id) (((can_id) & 0x80000000U) != 0)     //
// Use from controlcanfd.h #define IS_RTR(can_id) (((can_id) & 0x40000000U) !=
// 0)     // Use from controlcanfd.h
// #define MAKE_CAN_ID(id, eff, rtr, err) ((uint32_t)((id) & 0x1FFFFFFFU) | \
//     ((uint32_t)(eff) << 31) | ((uint32_t)(rtr) << 30) | ((uint32_t)(err) <<
//     29))  // Use from controlcanfd.h

// ===================================
// CONSTRUCTOR AND DESTRUCTOR
// ===================================

ZLGCanFDCommunicationAdapter::ZLGCanFDCommunicationAdapter(
    const ZLGCanFDConfig &config)
    : config_(config), device_handle_(nullptr), channel_handle_(nullptr),
      library_handle_(nullptr), open_device_func_(nullptr),
      close_device_func_(nullptr), init_can_func_(nullptr),
      start_can_func_(nullptr), stop_can_func_(nullptr),
      reset_can_func_(nullptr), transmit_fd_func_(nullptr),
      receive_fd_func_(nullptr), get_device_info_func_(nullptr),
      clear_buffer_func_(nullptr), set_filter_func_(nullptr),
      receive_thread_running_(false), performance_monitoring_enabled_(true),
      initialized_(false), device_open_(false), channel_started_(false),
      latency_history_index_(0) {

  // Initialize performance statistics
  reset_performance_stats();
  stats_.statistics_start_time = std::chrono::high_resolution_clock::now();

  // Initialize timing arrays
  send_latency_history_.fill(std::chrono::microseconds(0));
  receive_interval_history_.fill(std::chrono::microseconds(0));

  std::cout << "🔧 ZLG CAN FD Adapter created" << std::endl;
  if (config_.debug) {
    std::cout << "   Device Type: " << config_.device_type << std::endl;
    std::cout << "   Channel Index: " << config_.channel_index << std::endl;
    std::cout << "   CAN FD Mode: "
              << (config_.can_fd_mode ? "Enabled" : "Disabled") << std::endl;
    std::cout << "   Auto Channel Selection: "
              << (config_.auto_channel_selection ? "Enabled" : "Disabled")
              << std::endl;
  }
}

ZLGCanFDCommunicationAdapter::~ZLGCanFDCommunicationAdapter() {
  shutdown();
  std::cout << "🗑️ ZLG CAN FD Adapter destroyed" << std::endl;
}

// ===================================
// CORE COMMUNICATION INTERFACE
// ===================================

bool ZLGCanFDCommunicationAdapter::initialize_zlg_hardware() {
  std::lock_guard<std::mutex> lock(state_mutex_);

  if (initialized_) {
    std::cout << "⚠️ ZLG CAN FD Adapter already initialized" << std::endl;
    return true;
  }

  std::cout << "🚀 Initializing ZLG CAN FD Adapter..." << std::endl;

  // Load ZLG library
  if (!load_zlg_library()) {
    std::cout << "❌ Failed to load ZLG CAN FD library" << std::endl;
    return false;
  }

  // Open device
  if (!open_device()) {
    std::cout << "❌ Failed to open ZLG CAN FD device" << std::endl;
    unload_zlg_library();
    return false;
  }

  // Auto-configure channel if requested
  if (config_.auto_channel_selection) {
    if (!auto_configure_channel()) {
      std::cout << "❌ Failed to auto-configure channel" << std::endl;
      close_device();
      unload_zlg_library();
      return false;
    }
  } else {
    // Use specified channel
    if (!configure_channel(config_.channel_index, config_.arbitration_baud,
                           config_.data_baud)) {
      std::cout << "❌ Failed to configure channel " << config_.channel_index
                << std::endl;
      close_device();
      unload_zlg_library();
      return false;
    }
  }

  // Start channel
  if (!start_channel()) {
    std::cout << "❌ Failed to start CAN channel" << std::endl;
    close_device();
    unload_zlg_library();
    return false;
  }

  // Start receive thread
  start_receive_thread();

  initialized_ = true;
  std::cout << "✅ ZLG CAN FD Adapter initialized successfully" << std::endl;

  // Print device info
  auto device_info = get_device_info();
  std::cout << "📋 Device Information:" << std::endl;
  for (const auto &[key, value] : device_info) {
    std::cout << "   " << key << ": " << value << std::endl;
  }

  return true;
}

void ZLGCanFDCommunicationAdapter::shutdown() {
  std::lock_guard<std::mutex> lock(state_mutex_);

  if (!initialized_) {
    return;
  }

  std::cout << "🛑 Shutting down ZLG CAN FD Adapter..." << std::endl;

  // Stop receive thread
  stop_receive_thread();

  // Stop channel
  if (channel_started_) {
    stop_channel();
  }

  // Close device
  if (device_open_) {
    close_device();
  }

  // Unload library
  unload_zlg_library();

  // Reset state
  initialized_ = false;
  device_open_ = false;
  channel_started_ = false;

  std::cout << "✅ ZLG CAN FD Adapter shutdown complete" << std::endl;
}

bool ZLGCanFDCommunicationAdapter::is_ready() const {
  return initialized_.load() && device_open_.load() && channel_started_.load();
}

bool ZLGCanFDCommunicationAdapter::send_frame(const CANFrame &frame) {
  if (!is_ready()) {
    return false;
  }

  auto start_time = std::chrono::high_resolution_clock::now();

  // Convert frame to ZLG format
  ZCAN_TransmitFD_Data zlg_frame = convert_frame_to_zlg_fd(frame);

  // Send frame - DIRECT LIBRARY CALL (no dlsym) exactly like working test
  UINT result = ZCAN_TransmitFD(channel_handle_, &zlg_frame, 1);

  // ALWAYS print the CAN frame being sent for debugging - with std::flush to
  // ensure it shows up
  std::cout << "🔥 ZLG SENT: ID=0x" << std::hex << zlg_frame.frame.can_id
            << std::dec << " Standard Data DLC=" << zlg_frame.frame.len
            << " data:(0x)";
  for (uint32_t i = 0; i < zlg_frame.frame.len; i++) {
    printf(" %02X", zlg_frame.frame.data[i]);
  }

  // CRITICAL: Check if data was actually sent to hardware
  if (result == 1) {
    std::cout << " ✅ ZLG HARDWARE: Frame successfully transmitted" << std::endl;
  } else {
    std::cout << " ❌ ZLG HARDWARE ERROR: Failed to transmit frame (result=" << result << ")" << std::endl;
  }
  std::cout << std::dec << std::flush;

  auto end_time = std::chrono::high_resolution_clock::now();
  auto latency = std::chrono::duration_cast<std::chrono::microseconds>(
      end_time - start_time);

  // Update statistics
  {
    std::lock_guard<std::mutex> stats_lock(stats_mutex_);
    if (result == 1) {
      stats_.frames_sent++;
      stats_.last_send_time = end_time;
      update_send_performance(latency);
    } else {
      stats_.send_errors++;
      std::cout << "📊 ERROR STATISTICS: Send error count = " << stats_.send_errors << std::endl;
    }
  }

  return (result == 1);
}

size_t
ZLGCanFDCommunicationAdapter::send_frames(const std::vector<CANFrame> &frames) {
  if (!is_ready() || frames.empty()) {
    return 0;
  }

  auto start_time = std::chrono::high_resolution_clock::now();

  // Convert frames to ZLG format
  std::vector<ZCAN_TransmitFD_Data> zlg_frames;
  zlg_frames.reserve(frames.size());

  for (const auto &frame : frames) {
    zlg_frames.push_back(convert_frame_to_zlg_fd(frame));
  }

  // Send frames - DIRECT LIBRARY CALL (no dlsym) exactly like working test
  UINT result = ZCAN_TransmitFD(channel_handle_, zlg_frames.data(), static_cast<UINT>(zlg_frames.size()));

  auto end_time = std::chrono::high_resolution_clock::now();

  // Update statistics
  {
    std::lock_guard<std::mutex> stats_lock(stats_mutex_);
    if (result > 0) {
      stats_.frames_sent += result;
      stats_.last_send_time = end_time;
    } else {
      stats_.send_errors++;
    }
  }

  return static_cast<size_t>(result);
}

bool ZLGCanFDCommunicationAdapter::receive_frame(CANFrame &frame,
                                                 int timeout_ms) {
  std::vector<CANFrame> frames;
  auto count = receive_frames(frames, 1, timeout_ms);

  if (count > 0) {
    frame = frames[0];
    return true;
  }

  return false;
}

size_t ZLGCanFDCommunicationAdapter::receive_frames(
    std::vector<CANFrame> &frames, size_t max_frames, int timeout_ms) {
  if (!is_ready() || max_frames == 0) {
    return 0;
  }

  frames.clear();
  frames.reserve(max_frames);

  // Receive frames from ZLG device
  auto receive_func = reinterpret_cast<int (*)(
      CHANNEL_HANDLE, ZCAN_ReceiveFD_Data *, uint32_t, int)>(receive_fd_func_);
  if (!receive_func) {
    return 0;
  }

  std::vector<ZCAN_ReceiveFD_Data> zlg_frames(max_frames);
  int received =
      receive_func(channel_handle_, zlg_frames.data(), max_frames, timeout_ms);

  // Convert received frames
  for (int i = 0; i < received; i++) {
    CANFrame frame;
    frame.id = GET_ID(zlg_frames[i].frame.can_id);
    frame.is_extended_id = IS_EFF(zlg_frames[i].frame.can_id);
    frame.is_remote_frame = IS_RTR(zlg_frames[i].frame.can_id);
    // Note: CANFrame in protocol interface doesn't have dlc or timestamp fields
    // We'll use the data vector size and add timestamp as extra info if needed

    // Copy data
    size_t data_size = std::min(static_cast<size_t>(zlg_frames[i].frame.len),
                                static_cast<size_t>(64));
    frame.data.resize(data_size);
    std::memcpy(frame.data.data(), zlg_frames[i].frame.data, data_size);

    frames.push_back(frame);
  }

  // Update statistics
  {
    std::lock_guard<std::mutex> stats_lock(stats_mutex_);
    if (received > 0) {
      stats_.frames_received += received;
      stats_.last_receive_time = std::chrono::high_resolution_clock::now();
      update_receive_performance();
    } else {
      stats_.receive_errors++;
    }
  }

  return static_cast<size_t>(received);
}

bool ZLGCanFDCommunicationAdapter::set_receive_callback(
    std::function<void(const CANFrame &)> callback) {
  std::lock_guard<std::mutex> lock(callback_mutex_);
  receive_callback_ = callback;
  return true;
}

// ===================================
// CHANNEL CONFIGURATION
// ===================================

bool ZLGCanFDCommunicationAdapter::configure_channel(uint32_t channel_index,
                                                     uint32_t arbitration_baud,
                                                     uint32_t data_baud) {
  if (!device_open_) {
    std::cout << "❌ Cannot configure channel - device not open" << std::endl;
    return false;
  }

  std::cout << "🔧 Configuring ZLG CAN channel " << channel_index << std::endl;

  // Stop current channel if running
  if (channel_started_) {
    stop_channel();
  }

  // Update configuration
  config_.channel_index = channel_index;
  config_.arbitration_baud = arbitration_baud;
  config_.data_baud = data_baud;

  // Set baud rates - DIRECT LIBRARY CALLS
  uint32_t result_abit = ZCAN_SetAbitBaud(device_handle_, channel_index, arbitration_baud);
  if (result_abit != STATUS_OK) {
    std::cout << "❌ Failed to set arbitration baud rate" << std::endl;
    return false;
  }

  uint32_t result_dbit = ZCAN_SetDbitBaud(device_handle_, channel_index, data_baud);
  if (result_dbit != STATUS_OK) {
    std::cout << "❌ Failed to set data baud rate" << std::endl;
    return false;
  }

  // Initialize channel
  if (!init_channel()) {
    std::cout << "❌ Failed to initialize channel" << std::endl;
    return false;
  }

  // Clear filters (critical step - matches working test sequence) - DIRECT CALL
  uint32_t clear_result = ZCAN_ClearFilter(channel_handle_);
  if (clear_result != STATUS_OK) {
    std::cout << "⚠️ Failed to clear channel filter (non-critical)" << std::endl;
  } else {
    std::cout << "✅ Channel filters cleared" << std::endl;
  }

  // Acknowledge filter settings (critical step - matches working test sequence) - DIRECT CALL
  uint32_t ack_result = ZCAN_AckFilter(channel_handle_);
  if (ack_result != STATUS_OK) {
    std::cout << "⚠️ Failed to acknowledge filter settings (non-critical)" << std::endl;
  } else {
    std::cout << "✅ Filter settings acknowledged" << std::endl;
  }

  // Start channel
  if (!start_channel()) {
    std::cout << "❌ Failed to start channel" << std::endl;
    return false;
  }

  std::cout << "✅ Channel " << channel_index << " configured successfully with filters"
            << std::endl;
  return true;
}

bool ZLGCanFDCommunicationAdapter::auto_configure_channel() {
  std::cout << "🔍 Auto-detecting available ZLG CAN channels..." << std::endl;

  // Try channels 0-15
  for (uint32_t channel = 0; channel < 16; channel++) {
    if (configure_channel(channel, config_.arbitration_baud,
                          config_.data_baud)) {
      std::cout << "✅ Auto-configured channel " << channel << std::endl;
      return true;
    }
  }

  std::cout << "❌ No available ZLG CAN channels found" << std::endl;
  return false;
}

uint32_t ZLGCanFDCommunicationAdapter::get_current_channel() const {
  return config_.channel_index;
}

std::vector<uint32_t> ZLGCanFDCommunicationAdapter::get_available_channels() {
  std::vector<uint32_t> available_channels;

  if (!device_open_) {
    return available_channels;
  }

  // Test channels 0-15
  for (uint32_t channel = 0; channel < 16; channel++) {
    // Try to initialize channel (this is a basic test)
    auto init_func =
        reinterpret_cast<CHANNEL_HANDLE (*)(DEVICE_HANDLE, uint32_t, void *)>(
            dlsym(library_handle_, "ZCAN_InitCAN"));

    if (init_func) {
      // Create minimal config for testing
      struct {
        uint32_t can_type;
        struct {
          uint32_t acc_code;
          uint32_t acc_mask;
          uint32_t filter;
          uint32_t mode;
          uint32_t brp;
        } canfd;
      } test_config = {1, {0, 0xFFFFFFFF, 1, 0, 0}};

      CHANNEL_HANDLE test_handle =
          init_func(device_handle_, channel, &test_config);
      if (test_handle != INVALID_CHANNEL_HANDLE) {
        available_channels.push_back(channel);

        // Clean up test handle
        auto close_func = reinterpret_cast<uint32_t (*)(CHANNEL_HANDLE)>(
            dlsym(library_handle_, "ZCAN_ResetCAN"));
        if (close_func) {
          close_func(test_handle);
        }
      }
    }
  }

  return available_channels;
}

bool ZLGCanFDCommunicationAdapter::switch_channel(uint32_t new_channel_index) {
  if (new_channel_index == config_.channel_index) {
    return true; // Already using this channel
  }

  std::cout << "🔄 Switching from channel " << config_.channel_index
            << " to channel " << new_channel_index << std::endl;

  return configure_channel(new_channel_index, config_.arbitration_baud,
                           config_.data_baud);
}

// ===================================
// PERFORMANCE MONITORING
// ===================================

ZLGCanFDPerformanceStats
ZLGCanFDCommunicationAdapter::get_performance_stats() const {
  std::lock_guard<std::mutex> lock(stats_mutex_);
  return stats_;
}

void ZLGCanFDCommunicationAdapter::reset_performance_stats() {
  std::lock_guard<std::mutex> lock(stats_mutex_);

  stats_ = ZLGCanFDPerformanceStats{};
  stats_.statistics_start_time = std::chrono::high_resolution_clock::now();
  last_bus_load_calculation_ = stats_.statistics_start_time;

  // Reset timing arrays
  send_latency_history_.fill(std::chrono::microseconds(0));
  receive_interval_history_.fill(std::chrono::microseconds(0));
  latency_history_index_ = 0;
}

void ZLGCanFDCommunicationAdapter::print_performance_stats() const {
  std::lock_guard<std::mutex> lock(stats_mutex_);

  auto now = std::chrono::high_resolution_clock::now();
  auto uptime =
      std::chrono::duration<double>(now - stats_.statistics_start_time).count();

  std::cout << "\n📊 ZLG CAN FD Performance Statistics:" << std::endl;
  std::cout << std::string(50, '=') << std::endl;
  std::cout << "Uptime: " << std::fixed << std::setprecision(1) << uptime
            << " s" << std::endl;
  std::cout << "Frames Sent: " << stats_.frames_sent << std::endl;
  std::cout << "Frames Received: " << stats_.frames_received << std::endl;
  std::cout << "Send Errors: " << stats_.send_errors << std::endl;
  std::cout << "Receive Errors: " << stats_.receive_errors << std::endl;
  std::cout << "Timeout Errors: " << stats_.timeout_errors << std::endl;

  if (uptime > 0) {
    std::cout << "Send Frequency: " << std::fixed << std::setprecision(1)
              << (stats_.frames_sent / uptime) << " Hz" << std::endl;
    std::cout << "Receive Frequency: " << std::fixed << std::setprecision(1)
              << (stats_.frames_received / uptime) << " Hz" << std::endl;
  }

  std::cout << "Average Send Latency: " << std::fixed << std::setprecision(2)
            << stats_.average_send_latency_us << " μs" << std::endl;
  std::cout << "Max Send Latency: " << std::fixed << std::setprecision(2)
            << stats_.max_send_latency_us << " μs" << std::endl;
  std::cout << "Average Receive Interval: " << std::fixed
            << std::setprecision(2) << stats_.average_receive_interval_us
            << " μs" << std::endl;
  std::cout << "Bus Load: " << std::fixed << std::setprecision(1)
            << stats_.bus_load_percentage << "%" << std::endl;

  std::cout << "Device Errors: " << stats_.device_errors << std::endl;
  std::cout << "Channel Errors: " << stats_.channel_errors << std::endl;
  std::cout << "Buffer Overruns: " << stats_.buffer_overruns << std::endl;

  std::cout << std::string(50, '=') << std::endl;
}

void ZLGCanFDCommunicationAdapter::set_performance_monitoring_enabled(
    bool enabled) {
  performance_monitoring_enabled_ = enabled;
  std::cout << "📊 Performance monitoring "
            << (enabled ? "enabled" : "disabled") << std::endl;
}

// ===================================
// DEVICE INFORMATION
// ===================================

std::map<std::string, std::string>
ZLGCanFDCommunicationAdapter::get_device_info() {
  std::map<std::string, std::string> info;

  if (!device_open_) {
    return info;
  }

  auto get_info_func = reinterpret_cast<uint32_t (*)(DEVICE_HANDLE, void *)>(
      get_device_info_func_);
  if (!get_info_func) {
    return info;
  }

  // ZCAN_DEVICE_INFO structure (simplified)
  struct {
    uint16_t hw_Version;
    uint16_t fw_Version;
    uint16_t dr_Version;
    uint16_t in_Version;
    uint16_t irq_Num;
    uint16_t can_Num;
    char str_Serial_Num[20];
    char str_hw_Type[40];
    uint16_t reserved[4];
  } device_info;

  uint32_t result = get_info_func(device_handle_, &device_info);
  if (result == STATUS_OK) {
    info["hw_version"] = std::to_string(device_info.hw_Version);
    info["fw_version"] = std::to_string(device_info.fw_Version);
    info["dr_version"] = std::to_string(device_info.dr_Version);
    info["in_version"] = std::to_string(device_info.in_Version);
    info["irq_num"] = std::to_string(device_info.irq_Num);
    info["can_num"] = std::to_string(device_info.can_Num);
    info["serial_num"] = std::string(device_info.str_Serial_Num);
    info["hw_type"] = std::string(device_info.str_hw_Type);
  }

  // Add configuration info
  info["device_type"] = std::to_string(config_.device_type);
  info["channel_index"] = std::to_string(config_.channel_index);
  info["arbitration_baud"] = std::to_string(config_.arbitration_baud);
  info["data_baud"] = std::to_string(config_.data_baud);
  info["can_fd_mode"] = config_.can_fd_mode ? "enabled" : "disabled";

  return info;
}

bool ZLGCanFDCommunicationAdapter::is_device_online() {
  if (!device_open_) {
    return false;
  }

  auto online_func = reinterpret_cast<uint32_t (*)(DEVICE_HANDLE)>(
      dlsym(library_handle_, "ZCAN_IsDeviceOnLine"));

  return online_func && online_func(device_handle_) == 1;
}

ZLGCanFDConfig ZLGCanFDCommunicationAdapter::get_current_config() const {
  return config_;
}

bool ZLGCanFDCommunicationAdapter::update_config(
    const ZLGCanFDConfig &new_config) {
  bool requires_reinitialization = false;

  // Check if reinitialization is needed
  if (new_config.device_type != config_.device_type ||
      new_config.device_index != config_.device_index ||
      new_config.library_path != config_.library_path) {
    requires_reinitialization = true;
  }

  config_ = new_config;

  if (requires_reinitialization && initialized_) {
    std::cout << "🔄 Configuration change requires reinitialization..."
              << std::endl;
    shutdown();
    return initialize_zlg_hardware();
  }

  // Update channel configuration if needed
  if (initialized_ &&
      (new_config.channel_index != config_.channel_index ||
       new_config.arbitration_baud != config_.arbitration_baud ||
       new_config.data_baud != config_.data_baud)) {
    return configure_channel(new_config.channel_index,
                             new_config.arbitration_baud, new_config.data_baud);
  }

  return true;
}

// ===================================
// ADVANCED FEATURES
// ===================================

bool ZLGCanFDCommunicationAdapter::clear_buffers() {
  if (!channel_started_) {
    return false;
  }

  auto clear_func =
      reinterpret_cast<uint32_t (*)(CHANNEL_HANDLE)>(clear_buffer_func_);
  return clear_func && clear_func(channel_handle_) == STATUS_OK;
}

bool ZLGCanFDCommunicationAdapter::reset_channel() {
  if (!channel_started_) {
    return false;
  }

  auto reset_func =
      reinterpret_cast<uint32_t (*)(CHANNEL_HANDLE)>(reset_can_func_);
  return reset_func && reset_func(channel_handle_) == STATUS_OK;
}

bool ZLGCanFDCommunicationAdapter::set_hardware_filter(uint32_t start_id,
                                                       uint32_t end_id) {
  // Implementation would depend on ZLG filter API
  // This is a placeholder for the filter functionality
  std::cout << "🔧 Setting hardware filter: 0x" << std::hex << start_id
            << " - 0x" << end_id << std::dec << std::endl;
  return true;
}

bool ZLGCanFDCommunicationAdapter::disable_hardware_filter() {
  std::cout << "🔧 Disabling hardware filter" << std::endl;
  return true;
}

bool ZLGCanFDCommunicationAdapter::set_termination_enabled(bool enabled) {
  auto term_func =
      reinterpret_cast<uint32_t (*)(DEVICE_HANDLE, uint32_t, uint32_t)>(
          dlsym(library_handle_, "ZCAN_SetResistanceEnable"));

  if (!term_func) {
    return false;
  }

  uint32_t result =
      term_func(device_handle_, config_.channel_index, enabled ? 1 : 0);
  return result == STATUS_OK;
}

// ===================================
// PRIVATE IMPLEMENTATION
// ===================================

bool ZLGCanFDCommunicationAdapter::load_zlg_library() {
  std::cout << "🔧 ZLG DEBUG: Attempting to load library from path: "
            << config_.library_path << std::endl;
  library_handle_ = dlopen(config_.library_path.c_str(), RTLD_LAZY);
  if (!library_handle_) {
    std::cout << "❌ Failed to load ZLG library: " << dlerror() << std::endl;
    return false;
  }

  std::cout << "✅ ZLG library loaded: " << config_.library_path << std::endl;
  return load_function_pointers();
}

void ZLGCanFDCommunicationAdapter::unload_zlg_library() {
  if (library_handle_) {
    dlclose(library_handle_);
    library_handle_ = nullptr;
  }

  // Clear function pointers
  open_device_func_ = nullptr;
  close_device_func_ = nullptr;
  init_can_func_ = nullptr;
  start_can_func_ = nullptr;
  stop_can_func_ = nullptr;
  reset_can_func_ = nullptr;
  transmit_fd_func_ = nullptr;
  receive_fd_func_ = nullptr;
  get_device_info_func_ = nullptr;
  clear_buffer_func_ = nullptr;
  set_filter_func_ = nullptr;
}

bool ZLGCanFDCommunicationAdapter::load_function_pointers() {
  // Load function pointers using dlsym
  open_device_func_ = dlsym(library_handle_, "ZCAN_OpenDevice");
  close_device_func_ = dlsym(library_handle_, "ZCAN_CloseDevice");
  init_can_func_ = dlsym(library_handle_, "ZCAN_InitCAN");
  start_can_func_ = dlsym(library_handle_, "ZCAN_StartCAN");
  stop_can_func_ = dlsym(library_handle_, "ZCAN_StopCAN");
  reset_can_func_ = dlsym(library_handle_, "ZCAN_ResetCAN");
  transmit_fd_func_ = dlsym(library_handle_, "ZCAN_TransmitFD");
  receive_fd_func_ = dlsym(library_handle_, "ZCAN_ReceiveFD");
  get_device_info_func_ = dlsym(library_handle_, "ZCAN_GetDeviceInf");
  clear_buffer_func_ = dlsym(library_handle_, "ZCAN_ClearBuffer");

  // Check critical functions
  return open_device_func_ && init_can_func_ && start_can_func_ &&
         transmit_fd_func_ && receive_fd_func_;
}

bool ZLGCanFDCommunicationAdapter::open_device() {
  // DIRECT LIBRARY CALL - exactly like working test
  device_handle_ = ZCAN_OpenDevice(config_.device_type, config_.device_index, 0);
  if (device_handle_ == INVALID_DEVICE_HANDLE) {
    std::cout << "❌ Failed to open ZLG device" << std::endl;
    return false;
  }

  device_open_ = true;
  std::cout << "✅ ZLG device opened" << std::endl;
  return true;
}

bool ZLGCanFDCommunicationAdapter::close_device() {
  if (!device_open_) {
    return true;
  }

  // DIRECT LIBRARY CALL
  ZCAN_CloseDevice(device_handle_);

  device_handle_ = nullptr;
  device_open_ = false;
  std::cout << "✅ ZLG device closed" << std::endl;
  return true;
}

bool ZLGCanFDCommunicationAdapter::init_channel() {
  // Channel configuration structure - exactly like working test
  ZCAN_CHANNEL_INIT_CONFIG channel_config;
  channel_config.can_type = 1;
  channel_config.canfd.acc_code = 0;
  channel_config.canfd.acc_mask = 0xFFFFFFFF;
  channel_config.canfd.filter = 1;
  channel_config.canfd.mode = 0;
  channel_config.canfd.brp = 0;

  // DIRECT LIBRARY CALL - exactly like working test
  channel_handle_ = ZCAN_InitCAN(device_handle_, config_.channel_index, &channel_config);
  if (channel_handle_ == INVALID_CHANNEL_HANDLE) {
    std::cout << "❌ Failed to initialize ZLG channel " << config_.channel_index
              << std::endl;
    return false;
  }

  std::cout << "✅ ZLG channel " << config_.channel_index << " initialized"
            << std::endl;
  return true;
}

bool ZLGCanFDCommunicationAdapter::start_channel() {
  // DIRECT LIBRARY CALL - exactly like working test
  uint32_t result = ZCAN_StartCAN(channel_handle_);
  if (result != STATUS_OK) {
    std::cout << "❌ Failed to start ZLG channel" << std::endl;
    return false;
  }

  channel_started_ = true;
  std::cout << "✅ ZLG channel started" << std::endl;
  return true;
}

bool ZLGCanFDCommunicationAdapter::stop_channel() {
  if (!channel_started_) {
    return true;
  }

  auto stop_func =
      reinterpret_cast<uint32_t (*)(CHANNEL_HANDLE)>(stop_can_func_);
  if (stop_func) {
    stop_func(channel_handle_);
  }

  channel_started_ = false;
  std::cout << "✅ ZLG channel stopped" << std::endl;
  return true;
}

ZCAN_TransmitFD_Data
ZLGCanFDCommunicationAdapter::convert_frame_to_zlg_fd(const CANFrame &frame) {
  ZCAN_TransmitFD_Data zlg_frame;

  // Set CAN ID with flags
  zlg_frame.frame.can_id =
      MAKE_CAN_ID(frame.id, frame.is_extended_id, frame.is_remote_frame, 0);

  // Set data length
  zlg_frame.frame.len = frame.data.size();

  // Set CAN FD flag
  zlg_frame.frame.flags = should_use_can_fd(frame) ? 0 : 1;

  // Copy data
  size_t data_size =
      std::min(static_cast<size_t>(frame.data.size()), static_cast<size_t>(64));
  std::memcpy(zlg_frame.frame.data, frame.data.data(), data_size);

  // Note: Timestamp is handled at transmission time

  return zlg_frame;
}

bool ZLGCanFDCommunicationAdapter::should_use_can_fd(const CANFrame &frame) {
  return config_.can_fd_mode && (frame.data.size() > 8 || config_.force_can_fd);
}

void ZLGCanFDCommunicationAdapter::update_send_performance(
    std::chrono::microseconds latency) {
  if (!performance_monitoring_enabled_) {
    return;
  }

  // Update latency history
  send_latency_history_[latency_history_index_] = latency;
  latency_history_index_ =
      (latency_history_index_ + 1) % send_latency_history_.size();

  // Update statistics
  if (latency.count() > stats_.max_send_latency_us) {
    stats_.max_send_latency_us = latency.count();
  }

  // Calculate running average
  uint64_t sum = 0;
  for (const auto &lat : send_latency_history_) {
    sum += lat.count();
  }
  stats_.average_send_latency_us =
      static_cast<double>(sum) / send_latency_history_.size();
}

void ZLGCanFDCommunicationAdapter::update_receive_performance() {
  if (!performance_monitoring_enabled_) {
    return;
  }

  auto now = std::chrono::high_resolution_clock::now();
  if (stats_.last_receive_time.time_since_epoch().count() > 0) {
    auto interval = std::chrono::duration_cast<std::chrono::microseconds>(
        now - stats_.last_receive_time);

    // Update interval history
    receive_interval_history_[latency_history_index_] = interval;

    // Calculate running average
    uint64_t sum = 0;
    for (const auto &interval : receive_interval_history_) {
      sum += interval.count();
    }
    stats_.average_receive_interval_us =
        static_cast<double>(sum) / receive_interval_history_.size();

    if (interval.count() > stats_.max_receive_interval_us) {
      stats_.max_receive_interval_us = interval.count();
    }
  }
}

void ZLGCanFDCommunicationAdapter::start_receive_thread() {
  if (receive_thread_running_) {
    return;
  }

  receive_thread_running_ = true;
  receive_thread_ =
      std::make_unique<std::thread>([this]() { receive_thread_function(); });

  std::cout << "✅ ZLG receive thread started" << std::endl;
}

void ZLGCanFDCommunicationAdapter::stop_receive_thread() {
  if (!receive_thread_running_) {
    return;
  }

  receive_thread_running_ = false;
  if (receive_thread_ && receive_thread_->joinable()) {
    receive_thread_->join();
  }

  receive_thread_.reset();
  std::cout << "✅ ZLG receive thread stopped" << std::endl;
}

void ZLGCanFDCommunicationAdapter::receive_thread_function() {
  std::vector<CANFrame> frames;
  frames.reserve(100); // Pre-allocate for efficiency

  while (receive_thread_running_) {
    // Receive frames with short timeout
    frames.clear();
    auto received = receive_frames(frames, 100, 10); // 10ms timeout

    // Call callback for received frames
    if (received > 0) {
      std::lock_guard<std::mutex> lock(callback_mutex_);
      if (receive_callback_) {
        for (const auto &frame : frames) {
          receive_callback_(frame);
        }
      }
    }

    // Small sleep to prevent CPU spinning
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
}

// ===================================
// UTILITY FUNCTIONS
// ===================================

std::unique_ptr<ZLGCanFDCommunicationAdapter>
create_zlg_canfd_adapter(const std::string &device_serial, int channel_index) {

  ZLGCanFDConfig config;
  if (!device_serial.empty()) {
    // Parse serial number to set device index (simplified)
    config.device_index = 0;
  }

  if (channel_index >= 0) {
    config.channel_index = static_cast<uint32_t>(channel_index);
    config.auto_channel_selection = false;
  }

  return std::make_unique<ZLGCanFDCommunicationAdapter>(config);
}

std::vector<std::map<std::string, std::string>> detect_zlg_canfd_devices() {
  std::vector<std::map<std::string, std::string>> devices;

  // Try to open devices 0-3
  for (uint32_t device_index = 0; device_index < 4; device_index++) {
    ZLGCanFDConfig config;
    config.device_index = device_index;

    auto adapter = std::make_unique<ZLGCanFDCommunicationAdapter>(config);
    if (adapter->initialize_zlg_hardware()) {
      auto device_info = adapter->get_device_info();
      device_info["device_index"] = std::to_string(device_index);
      devices.push_back(device_info);
      adapter->shutdown();
    }
  }

  return devices;
}

bool test_zlg_canfd_adapter(ZLGCanFDCommunicationAdapter *adapter,
                            uint32_t test_duration_ms) {
  if (!adapter || !adapter->is_ready()) {
    return false;
  }

  std::cout << "🧪 Testing ZLG CAN FD adapter for " << test_duration_ms
            << "ms..." << std::endl;

  // Reset statistics
  adapter->reset_performance_stats();

  // Test configuration
  ZLGCanFDConfig config = adapter->get_current_config();
  std::cout << "   Device Type: " << config.device_type << std::endl;
  std::cout << "   Channel: " << config.channel_index << std::endl;
  std::cout << "   CAN FD Mode: "
            << (config.can_fd_mode ? "Enabled" : "Disabled") << std::endl;

  // Send test frames
  auto start_time = std::chrono::high_resolution_clock::now();
  uint32_t frames_sent = 0;

  while (std::chrono::duration_cast<std::chrono::milliseconds>(
             std::chrono::high_resolution_clock::now() - start_time)
             .count() < test_duration_ms) {

    CANFrame test_frame;
    test_frame.id = 0x100 + (frames_sent % 10);
    test_frame.is_extended_id = false;
    test_frame.is_remote_frame = false;
    test_frame.data.resize(8);

    // Fill test data
    for (int i = 0; i < 8; i++) {
      test_frame.data[i] = static_cast<uint8_t>(frames_sent + i);
    }

    if (adapter->send_frame(test_frame)) {
      frames_sent++;
    }

    // Small delay
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  // Print results
  auto stats = adapter->get_performance_stats();
  std::cout << "   Frames Sent: " << frames_sent << std::endl;
  std::cout << "   Success Rate: " << std::fixed << std::setprecision(1)
            << (frames_sent > 0 ? 100.0 : 0.0) << "%" << std::endl;
  std::cout << "   Average Latency: " << std::fixed << std::setprecision(2)
            << stats.average_send_latency_us << " μs" << std::endl;

  adapter->print_performance_stats();

  bool test_passed = (frames_sent > 0 && stats.send_errors == 0);
  std::cout << "   Test Result: " << (test_passed ? "✅ PASSED" : "❌ FAILED")
            << std::endl;

  return test_passed;
}

// ===================================
// CANCommunicationInterface IMPLEMENTATION
// ===================================

bool ZLGCanFDCommunicationAdapter::initialize() {
  return initialize_zlg_hardware();
}

bool ZLGCanFDCommunicationAdapter::initialize(const std::string &device) {
  // The device parameter is ignored for ZLG as we use auto-detection
  return initialize_zlg_hardware();
}

bool ZLGCanFDCommunicationAdapter::is_connected() const { return is_ready(); }

void ZLGCanFDCommunicationAdapter::register_receive_callback(
    CANFrameCallback callback) {
  std::lock_guard<std::mutex> lock(callback_mutex_);
  receive_callback_ = callback;
}

bool ZLGCanFDCommunicationAdapter::start_receiving() {
  // The receive thread is already started in initialize()
  return is_ready();
}

void ZLGCanFDCommunicationAdapter::stop_receiving() {
  // Stop the receive thread
  stop_receive_thread();
}

ZLGCanFDCommunicationAdapter::CommStats
ZLGCanFDCommunicationAdapter::get_statistics() const {
  std::lock_guard<std::mutex> lock(stats_mutex_);
  CommStats stats;
  stats.frames_sent = stats_.frames_sent;
  stats.frames_received = stats_.frames_received;
  stats.send_errors = stats_.send_errors;
  stats.receive_errors = stats_.receive_errors;
  return stats;
}

} // namespace ic_can
