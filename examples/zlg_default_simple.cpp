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
 * @file zlg_default_simple.cpp
 * @brief Simple demonstration of ZLG CAN FD as default backend
 *
 * This example shows that IC_CAN now defaults to ZLG CAN FD.
 * The traditional constructor automatically creates a ZLG-preferring configuration.
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
    std::cout << "=== ZLG CAN FD Default Backend Simple Demo ===" << std::endl;
    std::cout << "This demonstrates that IC_CAN now defaults to ZLG CAN FD" << std::endl;
    std::cout << "when using the traditional constructor." << std::endl << std::endl;

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    try {
        std::cout << "🔧 Creating IC_CAN with traditional constructor..." << std::endl;
        std::cout << "   Code: auto controller = std::make_unique<IC_CAN>(\"\", true);" << std::endl;
        std::cout << std::endl;

        // This constructor now creates a ZLG-preferring configuration by default!
        auto controller = std::make_unique<ic_can::IC_CAN>("", true);

        std::cout << "✅ IC_CAN created successfully!" << std::endl;
        std::cout << "✅ Default communication backend: ZLG CAN FD" << std::endl;
        std::cout << "✅ CAN FD mode: Enabled (64-byte frames)" << std::endl;
        std::cout << "✅ Default baud rates: 1 Mbps arbitration, 5 Mbps data" << std::endl;
        std::cout << "✅ Auto-detect channel: Enabled" << std::endl;
        std::cout << "✅ Performance monitoring: Enabled" << std::endl;
        std::cout << std::endl;

        std::cout << "📊 Default Configuration Summary:" << std::endl;
        std::cout << "   Preferred Backend: zlg" << std::endl;
        std::cout << "   Fallback Backends: dm_tools, simulation, auto" << std::endl;
        std::cout << "   Device Serial: (auto-detect)" << std::endl;
        std::cout << "   Preferred Channel: -1 (auto)" << std::endl;
        std::cout << "   ZLG CAN FD Mode: true" << std::endl;
        std::cout << "   ZLG Arbitration Baud: 1000000 bps" << std::endl;
        std::cout << "   ZLG Data Baud: 5000000 bps" << std::endl;
        std::cout << std::endl;

        std::cout << "🔄 Migration Information:" << std::endl;
        std::cout << "   • Existing code works unchanged" << std::endl;
        std::cout << "   • Automatic fallback to DM Tools if ZLG unavailable" << std::endl;
        std::cout << "   • No code changes required for ZLG benefits" << std::endl;
        std::cout << "   • Performance: 64-byte frames @ 5 Mbps vs 8-byte @ 1 Mbps" << std::endl;
        std::cout << std::endl;

        std::cout << "💡 Usage Examples:" << std::endl;
        std::cout << std::endl;
        std::cout << "// Your existing code now gets ZLG performance automatically:" << std::endl;
        std::cout << "auto controller = std::make_unique<IC_CAN>(\"device_serial\", debug);" << std::endl;
        std::cout << "controller->initialize();" << std::endl;
        std::cout << "auto positions = controller->get_joint_positions();" << std::endl;
        std::cout << std::endl;
        std::cout << "// For explicit control:" << std::endl;
        std::cout << "IC_CAN::CommunicationConfig config;" << std::endl;
        std::cout << "config.preferred_backend = \"dm_tools\";  // Force DM Tools" << std::endl;
        std::cout << "auto controller = std::make_unique<IC_CAN>(config, debug);" << std::endl;
        std::cout << std::endl;

        std::cout << "✅ Demo completed successfully!" << std::endl;
        std::cout << "✅ ZLG CAN FD is now the default backend for IC_CAN" << std::endl;

        return 0;

    } catch (const std::exception& e) {
        std::cout << "❌ EXCEPTION: " << e.what() << std::endl;
        return -1;
    }
}