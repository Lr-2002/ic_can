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

/**
 * @file can_test.cpp
 * @brief Simple 10Hz CAN Communication Test Tool
 *
 * Sends specific command to servo motor (0x09) at 10Hz and prints send/receive
 * info.
 */

#include <atomic>
#include <chrono>
#include <csignal>
#include <iomanip>
#include <iostream>
#include <thread>

#include <ic_can/core/can_protocol_interface.hpp>
#include <ic_can/core/zlg_canfd_communication_adapter.hpp>

static std::atomic<bool> g_running(true);

void signal_handler(int signal) {
  std::cout << "\n🛑 Received signal " << signal << ", stopping..."
            << std::endl;
  g_running = false;
}

int main(int argc, char *argv[]) {
  std::cout << "🔧 10Hz CAN Test Tool - Send to 0x09" << std::endl;
  std::cout << "=====================================" << std::endl;

  // Set up signal handlers
  signal(SIGINT, signal_handler);
  signal(SIGTERM, signal_handler);

  try {
    // Create ZLG CAN FD communication adapter directly
    std::cout << "🚀 Initializing ZLG CAN FD adapter..." << std::endl;
    auto can_adapter = std::make_unique<ic_can::ZLGCanFDCommunicationAdapter>();

    // Initialize with default configuration
    if (!can_adapter->initialize("")) {
      std::cout << "❌ Failed to initialize ZLG CAN FD adapter" << std::endl;
      return -1;
    }

    // Set up receive callback to listen for responses
    bool response_pending = false;
    ic_can::CANFrame last_response;
    std::vector<ic_can::CANFrame> recent_responses;

    auto receive_callback = [&](const ic_can::CANFrame &frame) {
      if (frame.id == 0x07E5) { // Listen for servo response ID
        last_response = frame;
        response_pending = true;
        recent_responses.push_back(frame);

        // Keep only last 10 responses to avoid memory buildup
        if (recent_responses.size() > 10) {
          recent_responses.erase(recent_responses.begin());
        }
      }
    };

    can_adapter->set_receive_callback(receive_callback);

    std::cout << "✅ ZLG CAN FD adapter initialized" << std::endl;
    std::cout << "📡 Ready to send/receive CAN frames" << std::endl;
    std::cout << std::endl;

    // Commands to send alternately: 02 08 00 and 02 05 00
    std::vector<uint8_t> command_08 = {0x02, 0x08, 0x00, 0x00,
                                       0x00, 0x00, 0x00, 0x00};
    std::vector<uint8_t> command_05 = {0x02, 0x05, 0x00, 0x00,
                                       0x00, 0x00, 0x00, 0x00};

    std::cout << "🎯 Starting 10Hz alternating transmission to CAN ID 0x09"
              << std::endl;
    std::cout << "📤 Commands: 02 08 00 ↔ 02 05 00 (alternating)" << std::endl;
    std::cout << "📥 Expected response ID: 0x07E5" << std::endl;
    std::cout << "⏱️  Rate: 10Hz (100ms interval)" << std::endl;
    std::cout << "Press Ctrl+C to stop" << std::endl;
    std::cout << std::endl;

    // Print header
    std::cout << std::setw(12) << "Count" << std::setw(20) << "SEND"
              << std::setw(30) << "RECEIVE" << std::endl;
    std::cout << std::string(62, '-') << std::endl;

    uint64_t send_count = 0;
    uint64_t response_count = 0;
    auto start_time = std::chrono::high_resolution_clock::now();

    while (g_running) {
      auto loop_start = std::chrono::high_resolution_clock::now();

      // === SEND COMMAND (ALTERNATING) ===
      std::cout << std::dec << std::setw(12) << (++send_count) << "  ";

      // Alternate between commands
      std::vector<uint8_t> *current_command;
      if (send_count % 2 == 1) {
        current_command = &command_08;
        std::cout << "📤 ID:0x09 Data:";
        for (size_t i = 0; i < command_08.size(); ++i) {
          std::cout << std::hex << std::setw(2) << std::setfill('0')
                    << static_cast<int>(command_08[i]) << " ";
        }
        std::cout << std::dec << std::setfill(' ') << " [Position]  ";
      } else {
        current_command = &command_05;
        std::cout << "📤 ID:0x09 Data:";
        for (size_t i = 0; i < command_05.size(); ++i) {
          std::cout << std::hex << std::setw(2) << std::setfill('0')
                    << static_cast<int>(command_05[i]) << " ";
        }
        std::cout << std::dec << std::setfill(' ') << " [Status]  ";
      }

      // Create and send CAN frame
      ic_can::CANFrame frame(0x09, *current_command, false, false);

      try {
        bool sent = can_adapter->send_frame(frame);
        if (sent) {
          std::cout << "✅ Sent";
        } else {
          std::cout << "❌ Send failed";
        }
      } catch (const std::exception &e) {
        std::cout << "❌ Send error: " << e.what();
      }

      std::cout << std::endl;

      // === LISTEN FOR RESPONSE ===
      response_pending = false;
      auto response_start = std::chrono::high_resolution_clock::now();
      auto timeout = std::chrono::milliseconds(1); // 50ms timeout for response

      while (!response_pending && (std::chrono::high_resolution_clock::now() -
                                   response_start) < timeout) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
      }

      if (response_pending) {
        response_count++;
        std::cout << std::string(12, ' ') << std::string(20, ' ') << "📥 ID:0x"
                  << std::hex << std::setfill('0') << std::setw(4)
                  << last_response.id << std::dec << std::setfill(' ')
                  << " Data:";

        for (size_t i = 0; i < last_response.data.size(); ++i) {
          std::cout << std::hex << std::setw(2) << std::setfill('0')
                    << static_cast<int>(last_response.data[i]) << " ";
        }

        // Decode servo message type
        if (last_response.data.size() >= 3) {
          uint8_t cmd = last_response.data[0];
          uint8_t type = last_response.data[1];
          std::string msg_type = "";

          if (cmd == 0x02 && type == 0x08) {
            msg_type = " [Position/Status]";
          } else if (cmd == 0x02 && type == 0x05) {
            msg_type = " [Status/Error]";
          } else {
            msg_type = " [Unknown]";
          }

          std::cout << std::dec << std::setfill(' ') << msg_type
                    << "  ✅ Response #" << response_count;
        } else {
          std::cout << std::dec << std::setfill(' ') << "  ✅ Response #"
                    << response_count;
        }
        std::cout << std::endl;
      } else {
        std::cout << std::string(12, ' ') << std::string(20, ' ')
                  << "📥 No response within 50ms" << std::endl;
      }

      // === TIMING CONTROL ===
      auto loop_end = std::chrono::high_resolution_clock::now();
      auto loop_duration =
          std::chrono::duration_cast<std::chrono::milliseconds>(loop_end -
                                                                loop_start);
      auto target_duration = std::chrono::milliseconds(100); // 10Hz = 100ms

      if (loop_duration < target_duration) {
        std::this_thread::sleep_for(target_duration - loop_duration);
      } else {
        std::cout << "⚠️  Loop took " << loop_duration.count()
                  << "ms (over 100ms target)" << std::endl;
      }
    }

    // Print summary
    auto total_time = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::high_resolution_clock::now() - start_time);
    double actual_freq = static_cast<double>(send_count) / total_time.count();

    std::cout << std::endl;
    std::cout << "📊 Summary:" << std::endl;
    std::cout << "   Total sent: " << send_count << " commands" << std::endl;
    std::cout << "   Total responses: " << response_count << std::endl;
    std::cout << "   Response rate: " << std::fixed << std::setprecision(1)
              << (send_count > 0 ? (100.0 * response_count / send_count) : 0.0)
              << "%" << std::endl;
    std::cout << "   Duration: " << total_time.count() << " seconds"
              << std::endl;
    std::cout << "   Actual frequency: " << std::fixed << std::setprecision(2)
              << actual_freq << " Hz" << std::endl;

    can_adapter->shutdown();

  } catch (const std::exception &e) {
    std::cout << "❌ Error: " << e.what() << std::endl;
    return -1;
  }

  std::cout << "✅ Test completed" << std::endl;
  return 0;
}
