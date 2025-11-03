// Test for synchronous CAN send-receive system
#include <iostream>
#include <chrono>
#include <thread>
#include <ic_can/core/ic_can.hpp>

int main() {
    std::cout << "=== Synchronous CAN System Test ===" << std::endl;

    // Create IC_CAN controller with debug enabled
    auto controller = std::make_unique<ic_can::IC_CAN>("693D3DE86DF5940C8BC74A5B46A3CE2E", true);

    // Initialize system
    if (!controller->initialize()) {
        std::cout << "❌ FAILED: System initialization failed" << std::endl;
        return -1;
    }

    std::cout << "✅ System initialized successfully" << std::endl;
    std::cout << "🧪 Testing synchronous CAN send-receive pattern..." << std::endl;

    // Test the synchronous system by sending a few commands
    for (int i = 0; i < 3; i++) {
        std::cout << "\n--- Test " << (i+1) << " ---" << std::endl;

        // Send a refresh command (this should trigger synchronous communication)
        controller->refresh_all();

        // Wait a bit to see the response
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    std::cout << "\n✅ Synchronous CAN test completed" << std::endl;
    return 0;
}