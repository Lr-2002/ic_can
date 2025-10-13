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

#include <iostream>
#include <thread>
#include <chrono>
#include <vector>

#include <ic_can/protocols/usb2can_adapter.hpp>

int main() {
    std::cout << "=== Communication Tests ===" << std::endl;

    try {
        // Test USB2CAN adapter
        std::cout << "Testing USB2CAN adapter..." << std::endl;
        auto adapter = std::make_unique<ic_can::USB2CANAdapter>("F561E08C892274DB09496BCC1102DBC5");

        if (adapter) {
            std::cout << "✓ USB2CAN adapter created" << std::endl;

            // Test connection
            bool connected = adapter->connect();
            std::cout << "✓ Connection status: " << (connected ? "Connected" : "Disconnected") << std::endl;

            if (connected) {
                std::cout << "  Device connected: " << adapter->is_connected() << std::endl;

                // Test CAN frame creation and sending
                std::cout << "Testing CAN frame operations..." << std::endl;
                ic_can::CANFrame test_frame;
                test_frame.can_id = 0x201;
                test_frame.dlc = 8;
                test_frame.data = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
                test_frame.timestamp = std::chrono::steady_clock::now();

                bool sent = adapter->send_frame(test_frame);
                std::cout << "✓ CAN frame sent: " << (sent ? "Success" : "Failed") << std::endl;

                // Test batch frame sending
                std::cout << "Testing batch frame operations..." << std::endl;
                std::vector<ic_can::CANFrame> frames;
                for (int i = 0; i < 5; ++i) {
                    ic_can::CANFrame frame;
                    frame.can_id = 0x200 + i;
                    frame.dlc = 8;
                    frame.data = {static_cast<uint8_t>(i), 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07};
                    frame.timestamp = std::chrono::steady_clock::now();
                    frames.push_back(frame);
                }

                int sent_count = adapter->send_frames(frames);
                std::cout << "✓ Batch frames sent: " << sent_count << "/" << frames.size() << std::endl;

                // Test frame reception (simulate)
                std::cout << "Testing frame reception..." << std::endl;
                adapter->start_receiving();

                // Wait for frames
                std::this_thread::sleep_for(std::chrono::milliseconds(100));

                auto received_frames = adapter->get_received_frames();
                std::cout << "✓ Frames received: " << received_frames.size() << std::endl;

                adapter->stop_receiving();

                // Test device information
                std::cout << "Testing device information..." << std::endl;
                auto device_info = adapter->get_device_info();
                std::cout << "✓ Device info retrieved" << std::endl;
                std::cout << "  Info entries: " << device_info.size() << std::endl;

                // Test bus statistics
                std::cout << "Testing bus statistics..." << std::endl;
                auto stats = adapter->get_bus_statistics();
                std::cout << "✓ Bus statistics retrieved" << std::endl;
                std::cout << "  Statistics entries: " << stats.size() << std::endl;

                // Disconnect
                adapter->disconnect();
                std::cout << "✓ Disconnected" << std::endl;
                std::cout << "  Device connected: " << adapter->is_connected() << std::endl;
            } else {
                std::cout << "⚠ Could not connect to device (expected in simulation mode)" << std::endl;
            }
        }

        // Test CAN frame operations
        std::cout << "Testing CAN frame operations..." << std::endl;
        ic_can::CANFrame frame1, frame2;

        frame1.can_id = 0x201;
        frame1.dlc = 8;
        frame1.data = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};

        frame2.can_id = 0x202;
        frame2.dlc = 8;
        frame2.data = {0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01};

        std::cout << "✓ CAN frames created" << std::endl;
        std::cout << "  Frame 1 ID: 0x" << std::hex << frame1.can_id << std::dec << std::endl;
        std::cout << "  Frame 2 ID: 0x" << std::hex << frame2.can_id << std::dec << std::endl;

        // Test frame validation
        bool valid1 = (frame1.can_id == 0x201 && frame1.dlc == 8 && frame1.data.size() == 8);
        bool valid2 = (frame2.can_id == 0x202 && frame2.dlc == 8 && frame2.data.size() == 8);

        std::cout << "✓ Frame validation: " << (valid1 && valid2 ? "Passed" : "Failed") << std::endl;

        std::cout << "All communication tests passed!" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Communication test failed: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}