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
 * @file zlg_default_demo.cpp
 * @brief Demonstration of ZLG CAN FD as Default Backend
 *
 * This example demonstrates that IC_CAN now defaults to ZLG CAN FD
 * with automatic fallback to DM Tools if ZLG is unavailable.
 */

#include <chrono>
#include <iostream>
#include <ic_can/core/ic_can.hpp>
#include <thread>
#include <csignal>

static volatile bool g_running = true;

void signal_handler(int signal) {
    std::cout << "\n⚠️  Received signal " << signal << ", stopping demo..." << std::endl;
    g_running = false;
}

int main() {
    std::cout << "=== IC_CAN ZLG Default Backend Demo ===" << std::endl;
    std::cout << "This demo shows how IC_CAN now defaults to ZLG CAN FD" << std::endl;
    std::cout << "with automatic fallback to DM Tools." << std::endl << std::endl;

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    try {
        // Method 1: Traditional constructor - now defaults to ZLG!
        std::cout << "🔧 Method 1: Traditional constructor (now ZLG by default)" << std::endl;
        std::cout << "   auto controller = std::make_unique<IC_CAN>(device_serial, debug);" << std::endl;

        auto controller = std::make_unique<ic_can::IC_CAN>("", true); // Empty serial = auto-detect

        // Print communication status to see which backend is being used
        std::cout << "📊 Communication Status:" << std::endl;
        controller->print_communication_status();

        // Initialize the system
        std::cout << "\n🚀 Initializing system..." << std::endl;
        if (!controller->initialize()) {
            std::cout << "❌ Initialization failed" << std::endl;
            return -1;
        }
        std::cout << "✅ System initialized successfully!" << std::endl;

        // Show current backend after initialization
        std::cout << "\n📋 Active Backend: " << controller->get_current_backend() << std::endl;
        controller->print_channel_status();

        // Test basic functionality
        std::cout << "\n🧪 Testing basic functionality..." << std::endl;

        if (controller->refresh_all()) {
            std::cout << "✅ Motor refresh successful" << std::endl;

            // Get joint positions
            auto positions = controller->get_joint_positions();
            if (!positions.empty()) {
                std::cout << "✅ Joint positions retrieved (" << positions.size() << " motors)" << std::endl;
                std::cout << "   First 3 positions: [";
                for (size_t i = 0; i < std::min(size_t(3), positions.size()); ++i) {
                    std::cout << positions[i];
                    if (i < std::min(size_t(3), positions.size()) - 1) std::cout << ", ";
                }
                std::cout << "]" << std::endl;
            }
        } else {
            std::cout << "⚠️  Motor refresh failed (expected if no motors connected)" << std::endl;
        }

        std::cout << "\n🔄 Demonstrating backend switching..." << std::endl;

        // Show available backends
        auto available_backends = controller->get_available_backends();
        std::cout << "Available backends: [";
        for (size_t i = 0; i < available_backends.size(); ++i) {
            std::cout << available_backends[i];
            if (i < available_backends.size() - 1) std::cout << ", ";
        }
        std::cout << "]" << std::endl;

        // Test backend switching if multiple backends are available
        if (available_backends.size() > 1) {
            std::string current_backend = controller->get_current_backend();
            std::string next_backend = (current_backend == "zlg") ? "dm_tools" : "zlg";

            if (std::find(available_backends.begin(), available_backends.end(), next_backend) != available_backends.end()) {
                std::cout << "🔄 Switching from " << current_backend << " to " << next_backend << "..." << std::endl;

                if (controller->switch_communication_backend(next_backend)) {
                    std::cout << "✅ Successfully switched to " << next_backend << std::endl;
                    std::cout << "📋 New active backend: " << controller->get_current_backend() << std::endl;
                } else {
                    std::cout << "❌ Failed to switch backend" << std::endl;
                }
            }
        }

        std::cout << "\n✅ Demo completed successfully!" << std::endl;
        std::cout << "\n📋 Summary:" << std::endl;
        std::cout << "   - IC_CAN now defaults to ZLG CAN FD" << std::endl;
        std::cout << "   - Automatic fallback to DM Tools if ZLG unavailable" << std::endl;
        std::cout << "   - Runtime backend switching is supported" << std::endl;
        std::cout << "   - Multi-channel configuration available for ZLG" << std::endl;
        std::cout << "   - CAN FD mode (64-byte frames) enabled by default" << std::endl;

        controller->shutdown();
        return 0;

    } catch (const std::exception& e) {
        std::cout << "❌ EXCEPTION: " << e.what() << std::endl;
        return -1;
    }
}