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

#include "ic_can/core/can_bus_logger.hpp"
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <filesystem>
#include <cstdio>

namespace ic_can {

CANBusLogger::CANBusLogger(const std::string& log_directory)
    : logging_active_(false), log_directory_(log_directory) {
    if (log_directory_.empty()) {
        // Create default directory with timestamp
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        auto tm = *std::localtime(&time_t);

        char dir_name[100];
        strftime(dir_name, sizeof(dir_name), "can_bus_log_%Y%m%d_%H%M%S", &tm);
        log_directory_ = dir_name;
    }
}

CANBusLogger::~CANBusLogger() {
    if (is_logging()) {
        stop_logging();
    }
}

bool CANBusLogger::start_logging() {
    if (logging_active_.load()) {
        return true;  // Already logging
    }

    if (!initialize_logging()) {
        return false;
    }

    start_time_ = std::chrono::high_resolution_clock::now();
    logging_active_.store(true);

    std::cout << "📋 CAN Bus Logger started" << std::endl;
    std::cout << "📁 Log directory: " << log_directory_ << std::endl;
    std::cout << "📄 Files created:" << std::endl;
    std::cout << "   - can_frames.log (human readable)" << std::endl;
    std::cout << "   - can_frames.csv (machine readable)" << std::endl;

    return true;
}

void CANBusLogger::stop_logging() {
    if (!logging_active_.load()) {
        return;  // Not logging
    }

    logging_active_.store(false);

    // Write final statistics
    auto now = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration<double>(now - start_time_).count();

    can_log_file_ << "\n" << std::string(80, '=') << std::endl;
    can_log_file_ << "CAN BUS LOGGING SUMMARY" << std::endl;
    can_log_file_ << "Duration: " << std::fixed << std::setprecision(2) << duration << " seconds" << std::endl;

    auto stats = get_statistics();
    can_log_file_ << "Total Sent: " << stats.total_sent << std::endl;
    can_log_file_ << "Total Received: " << stats.total_received << std::endl;
    can_log_file_ << "Sent Frequency: " << std::setprecision(1) << stats.sent_frequency << " Hz" << std::endl;
    can_log_file_ << "Received Frequency: " << std::setprecision(1) << stats.received_frequency << " Hz" << std::endl;
    can_log_file_ << std::string(80, '=') << std::endl;

    close_log_files();

    std::cout << "🛑 CAN Bus Logger stopped" << std::endl;
    print_statistics();
}

void CANBusLogger::log_sent_frame(uint32_t can_id, bool extended_id, uint8_t dlc,
                                  const std::vector<uint8_t>& data) {
    if (!logging_active_.load()) {
        return;
    }

    CANFrameLog frame("TX", can_id, extended_id, dlc, data);

    {
        std::lock_guard<std::mutex> lock(buffer_mutex_);
        frame_buffer_.push_back(frame);
    }

    write_frame_to_files(frame);
}

void CANBusLogger::log_received_frame(uint32_t can_id, bool extended_id, uint8_t dlc,
                                     const std::vector<uint8_t>& data) {
    if (!logging_active_.load()) {
        return;
    }

    CANFrameLog frame("RX", can_id, extended_id, dlc, data);

    {
        std::lock_guard<std::mutex> lock(buffer_mutex_);
        frame_buffer_.push_back(frame);
    }

    write_frame_to_files(frame);
}

CANBusLogger::Statistics CANBusLogger::get_statistics() const {
    Statistics stats;

    if (!logging_active_.load() && frame_buffer_.empty()) {
        return stats;
    }

    auto now = std::chrono::high_resolution_clock::now();
    if (!frame_buffer_.empty()) {
        stats.uptime = std::chrono::duration<double>(now - frame_buffer_[0].timestamp);
    } else {
        stats.uptime = std::chrono::duration<double>(now - start_time_);
    }

    {
        std::lock_guard<std::mutex> lock(buffer_mutex_);
        for (const auto& frame : frame_buffer_) {
            if (frame.direction == "TX") {
                stats.total_sent++;
                stats.sent_per_id[frame.can_id]++;
            } else {
                stats.total_received++;
                stats.received_per_id[frame.can_id]++;
            }
        }
    }

    if (stats.uptime.count() > 0) {
        stats.sent_frequency = stats.total_sent / stats.uptime.count();
        stats.received_frequency = stats.total_received / stats.uptime.count();
    }

    return stats;
}

void CANBusLogger::print_statistics() const {
    auto stats = get_statistics();

    std::cout << "\n📊 CAN BUS STATISTICS:" << std::endl;
    std::cout << "Uptime: " << std::fixed << std::setprecision(2) << stats.uptime.count() << "s" << std::endl;
    std::cout << "Total Sent: " << stats.total_sent << std::endl;
    std::cout << "Total Received: " << stats.total_received << std::endl;
    std::cout << "Send Frequency: " << std::setprecision(1) << stats.sent_frequency << " Hz" << std::endl;
    std::cout << "Receive Frequency: " << std::setprecision(1) << stats.received_frequency << " Hz" << std::endl;

    std::cout << "\n📡 PER-MOTOR RECEIVE STATISTICS:" << std::endl;
    std::cout << "Motor | CAN ID  | Frames | Frequency" << std::endl;
    std::cout << "------|---------|--------|----------" << std::endl;

    // Group received frames by motor
    std::map<int, uint64_t> motor_receive_counts;
    for (const auto& [can_id, count] : stats.received_per_id) {
        int motor_id = can_id_to_motor_id(can_id);
        if (motor_id > 0) {
            motor_receive_counts[motor_id] += count;
        }
    }

    for (int motor = 1; motor <= 9; motor++) {
        uint64_t count = motor_receive_counts[motor];
        double frequency = 0.0;
        if (stats.uptime.count() > 0) {
            frequency = count / stats.uptime.count();
        }

        std::cout << std::setw(5) << motor << " | "
                  << std::setw(7) << "0x" << std::hex << (0x10 + motor) << std::dec << " | "
                  << std::setw(7) << count << " | "
                  << std::setw(8) << std::fixed << std::setprecision(1) << frequency << " Hz" << std::endl;
    }

    std::cout << std::dec << std::endl;  // Reset to decimal
}

int CANBusLogger::can_id_to_motor_id(uint32_t can_id) const {
    // DM motors 1-6: IDs 0x11-0x16
    if (can_id >= 0x11 && can_id <= 0x16) {
        return can_id - 0x10;  // 0x11->1, 0x16->6
    }

    // HT motor 7: IDs 0x700-0x7FF
    if (can_id >= 0x700 && can_id < 0x800) {
        return 7;
    }

    // HT motor 8: IDs 0x800-0x8FF
    if (can_id >= 0x800 && can_id < 0x900) {
        return 8;
    }

    // Servo motor 9: ID 0x19
    if (can_id == 0x19) {
        return 9;
    }

    return 0;  // Unknown CAN ID
}

bool CANBusLogger::initialize_logging() {
    if (!create_log_directory()) {
        return false;
    }

    if (!open_log_files()) {
        return false;
    }

    // Write CSV header
    csv_log_file_ << "Timestamp,Direction,CAN_ID,Extended,DLC,Data" << std::endl;

    return true;
}

bool CANBusLogger::create_log_directory() {
    try {
        std::cout << "🔧 CAN Logger: Creating directory: " << log_directory_ << std::endl;
        bool result = std::filesystem::create_directories(log_directory_);
        if (result) {
            std::cout << "✅ CAN Logger: Directory created successfully" << std::endl;
        } else {
            std::cout << "ℹ️ CAN Logger: Directory already exists or creation failed" << std::endl;
        }
        return true; // Always continue, directory may already exist
    } catch (const std::exception& e) {
        std::cerr << "❌ Failed to create log directory: " << e.what() << std::endl;
        return false;
    }
}

bool CANBusLogger::open_log_files() {
    std::cout << "🔧 CAN Logger: Opening log files..." << std::endl;

    // Open human readable log file
    std::string log_file_path = log_directory_ + "/can_frames.log";
    std::cout << "🔧 CAN Logger: Opening log file: " << log_file_path << std::endl;
    can_log_file_.open(log_file_path);
    if (!can_log_file_.is_open()) {
        std::cerr << "❌ Failed to open log file: " << log_file_path << std::endl;
        return false;
    }
    std::cout << "✅ CAN Logger: Log file opened successfully" << std::endl;

    // Open CSV file
    std::string csv_file_path = log_directory_ + "/can_frames.csv";
    std::cout << "🔧 CAN Logger: Opening CSV file: " << csv_file_path << std::endl;
    csv_log_file_.open(csv_file_path);
    if (!csv_log_file_.is_open()) {
        std::cerr << "❌ Failed to open CSV file: " << csv_file_path << std::endl;
        can_log_file_.close();
        return false;
    }
    std::cout << "✅ CAN Logger: CSV file opened successfully" << std::endl;

    return true;
}

void CANBusLogger::close_log_files() {
    if (can_log_file_.is_open()) {
        can_log_file_.close();
    }
    if (csv_log_file_.is_open()) {
        csv_log_file_.close();
    }
}

void CANBusLogger::write_frame_to_files(const CANFrameLog& frame) {
    if (!can_log_file_.is_open() || !csv_log_file_.is_open()) {
        return;
    }

    // Write to human readable log
    can_log_file_ << format_frame(frame) << std::endl;

    // Write to CSV
    csv_log_file_ << format_frame_csv(frame) << std::endl;
}

std::string CANBusLogger::format_frame(const CANFrameLog& frame) const {
    std::ostringstream oss;

    auto timestamp_us = std::chrono::duration_cast<std::chrono::microseconds>(
        frame.timestamp.time_since_epoch()).count();

    oss << std::fixed << std::setprecision(6) << (timestamp_us / 1000000.0) << "s "
        << "[" << frame.direction << "] ID=0x" << std::hex << std::setw(3) << std::setfill('0')
        << frame.can_id << std::dec << std::setfill(' ');

    if (frame.extended_id) {
        oss << " EXT";
    } else {
        oss << " STD";
    }

    oss << " DLC=" << static_cast<int>(frame.dlc) << " Data: ";

    for (size_t i = 0; i < frame.data.size(); i++) {
        oss << std::hex << std::setw(2) << std::setfill('0')
            << static_cast<int>(frame.data[i]);
        if (i < frame.data.size() - 1) {
            oss << " ";
        }
    }

    oss << std::dec << std::setfill(' ');

    return oss.str();
}

std::string CANBusLogger::format_frame_csv(const CANFrameLog& frame) const {
    std::ostringstream oss;

    auto timestamp_us = std::chrono::duration_cast<std::chrono::microseconds>(
        frame.timestamp.time_since_epoch()).count();

    oss << timestamp_us << ","
        << frame.direction << ","
        << "0x" << std::hex << frame.can_id << std::dec << ","
        << (frame.extended_id ? "1" : "0") << ","
        << static_cast<int>(frame.dlc) << ",\"";

    for (size_t i = 0; i < frame.data.size(); i++) {
        oss << std::hex << std::setw(2) << std::setfill('0')
            << static_cast<int>(frame.data[i]);
    }

    oss << std::dec << std::setfill(' ') << "\"";

    return oss.str();
}

} // namespace ic_can