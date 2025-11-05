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
#include <chrono>

namespace ic_can {

/**
 * @brief Standard CAN Frame Structure
 *
 * Unified CAN frame structure that supports both classic CAN (8 bytes)
 * and CAN FD (up to 64 bytes) frames.
 */
struct CANFrame {
  uint32_t id;                           ///< CAN identifier (11-bit or 29-bit)
  bool extended_id;                       ///< True for 29-bit extended identifier
  bool remote_frame;                      ///< True for remote transmission request
  uint8_t dlc;                           ///< Data length code (0-8 for classic CAN, 0-64 for CAN FD)
  std::vector<uint8_t> data;             ///< Data payload
  uint32_t timestamp;                    ///< Timestamp in microseconds

  CANFrame() : id(0), extended_id(false), remote_frame(false), dlc(0), timestamp(0) {
    data.resize(64, 0); // Maximum CAN FD data size
  }

  CANFrame(uint32_t can_id, bool extended = false, bool remote = false,
           uint8_t length = 8, const std::vector<uint8_t>& payload = {})
      : id(can_id), extended_id(extended), remote_frame(remote),
        dlc(length), timestamp(0) {
    data = payload;
    data.resize(64, 0); // Ensure maximum size
  }

  /**
   * @brief Get actual data size (considering DLC)
   * @return Actual number of data bytes
   */
  size_t get_data_size() const {
    return std::min(static_cast<size_t>(dlc), data.size());
  }

  /**
   * @brief Check if this is a CAN FD frame
   * @return true if frame uses CAN FD format
   */
  bool is_can_fd() const {
    return dlc > 8;
  }

  /**
   * @brief Clear frame data
   */
  void clear() {
    id = 0;
    extended_id = false;
    remote_frame = false;
    dlc = 0;
    timestamp = 0;
    std::fill(data.begin(), data.end(), 0);
  }
};

} // namespace ic_can