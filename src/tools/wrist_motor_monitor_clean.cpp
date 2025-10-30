/**
 * @file wrist_motor_monitor_clean.cpp
 * @brief Clean Wrist Motor Monitor - Only 48-byte pose set and 12-byte refresh
 *
 * Simplified monitoring tool using only the two protocols:
 * - 48-byte frames for setting positions
 * - 12-byte refresh commands for requesting positions
 */

#include "ic_can/core/ic_can.hpp"
#include "ic_can/core/wrist_component.hpp"
#include "ic_can/core/clean_ht_protocol.hpp"
#include <atomic>
#include <chrono>
#include <csignal>
#include <iomanip>
#include <iostream>
#include <thread>

// Global flag for graceful shutdown
std::atomic<bool> keep_running(true);

// Signal handler for Ctrl+C
void signal_handler(int signal) {
    if (signal == SIGINT) {
        std::cout << "\n🛑 Received interrupt signal, shutting down gracefully..." << std::endl;
        keep_running = false;
    }
}

void print_header() {
    std::cout << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Motor | Position (rad) | Position (deg)" << std::endl;
    std::cout << "------|---------------|----------------" << std::endl;
}

void print_motor_data(int motor_id, double pos_rad) {
    double pos_deg = pos_rad * 180.0 / M_PI;
    std::cout << std::fixed << std::setprecision(3);
    std::cout << std::setw(4) << motor_id << "   | " << std::setw(11) << pos_rad
              << " | " << std::setw(12) << pos_deg << std::endl;
}

int main(int argc, char *argv[]) {
    // Default parameters
    double frequency = 20.0; // Hz
    bool enable_position_control = false;

    // Parse command line arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "-h" || arg == "--help") {
            std::cout << "=== Clean HT Motor Monitor ===" << std::endl;
            std::cout << "Using only 48-byte pose set and 12-byte refresh protocols" << std::endl;
            std::cout << std::endl;
            std::cout << "Usage: " << argv[0] << " [options]" << std::endl;
            std::cout << "Options:" << std::endl;
            std::cout << "  -f <freq>     Update frequency in Hz (default: 20)" << std::endl;
            std::cout << "  -c            Enable position control (test mode)" << std::endl;
            std::cout << "  -h            Show this help message" << std::endl;
            return 0;
        } else if (arg == "-f" && i + 1 < argc) {
            frequency = std::stod(argv[++i]);
        } else if (arg == "-c") {
            enable_position_control = true;
        }
    }

    // Set up signal handler
    signal(SIGINT, signal_handler);

    std::cout << "=== Clean HT Motor Monitor ===" << std::endl;
    std::cout << "📊 Using clean protocols:" << std::endl;
    std::cout << "   - 48-byte frames for position control" << std::endl;
    std::cout << "   - 12-byte refresh for position requests" << std::endl;
    std::cout << "📊 Monitoring frequency: " << frequency << " Hz" << std::endl;
    if (enable_position_control) {
        std::cout << "⚠️  Position control ENABLED (test mode)" << std::endl;
    } else {
        std::cout << "👁️  Position monitoring only" << std::endl;
    }
    std::cout << "Press Ctrl+C to stop" << std::endl;
    std::cout << std::endl;

    try {
        // Initialize IC_CAN system
        std::cout << "🔧 Initializing IC_CAN system..." << std::endl;
        ic_can::IC_CAN ic_can("693D3DE86DF5940C8BC74A5B46A3CE2E", true);

        if (!ic_can.initialize()) {
            std::cerr << "❌ Failed to initialize IC_CAN system" << std::endl;
            return 1;
        }

        // Get wrist component
        auto &wrist = ic_can.get_wrist();
        wrist.set_debug_mode(true);

        // Enable motors
        std::cout << "⚡ Enabling wrist motors..." << std::endl;
        if (!ic_can.enable_all()) {
            std::cerr << "❌ Failed to enable motors" << std::endl;
            return 1;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        std::cout << "✅ System initialized" << std::endl;
        std::cout << "🚀 Starting clean HT motor monitoring..." << std::endl;

        // Monitoring loop
        auto start_time = std::chrono::steady_clock::now();
        auto last_refresh = start_time;
        auto last_control = start_time;
        const auto refresh_interval = std::chrono::duration<double>(1.0 / frequency);
        const auto control_interval = std::chrono::duration<double>(2.0); // Control every 2 seconds
        double phase = 0.0;

        print_header();

        while (keep_running) {
            auto current_time = std::chrono::steady_clock::now();

            // Send 12-byte refresh commands for position requests
            if (current_time - last_refresh >= refresh_interval) {
                ic_can::CleanHTMotorProtocol::send_ht_refresh_command(wrist);
                last_refresh = current_time;
            }

            // Send 48-byte frame commands for position control (if enabled)
            if (enable_position_control && (current_time - last_control >= control_interval)) {
                // Generate test positions (small sinusoidal motion)
                phase += 0.1;
                double m7_pos = 0.05 * std::sin(phase);      // Small amplitude for safety
                double m8_pos = 0.03 * std::cos(phase * 0.8); // Small amplitude for safety

                std::cout << "\n🎮 Test position control:" << std::endl;
                std::cout << "   Motor 7 target: " << std::fixed << std::setprecision(3)
                          << m7_pos << " rad (" << (m7_pos * 180.0 / M_PI) << "°)" << std::endl;
                std::cout << "   Motor 8 target: " << std::fixed << std::setprecision(3)
                          << m8_pos << " rad (" << (m8_pos * 180.0 / M_PI) << "°)" << std::endl;

                // Send 48-byte frame commands
                ic_can::CleanHTMotorProtocol::send_ht_frame_command(
                    wrist, m7_pos, 0.0, 0.0,    // Motor 7: pos, vel, torque
                    m8_pos, 0.0, 0.0,         // Motor 8: pos, vel, torque
                    30.0, 0.5);              // Gains: kp=30.0, kd=0.5

                last_control = current_time;
            }

            // Get and display current positions
            auto positions = wrist.get_positions();
            if (positions.size() >= 2) {
                print_header(); // Print header every cycle for clarity
                print_motor_data(7, positions[0]); // Motor 7
                print_motor_data(8, positions[1]); // Motor 8
                std::cout << "----------------------------------------" << std::endl;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        std::cout << "✅ Monitoring stopped successfully" << std::endl;

    } catch (const std::exception &e) {
        std::cerr << "❌ Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}