/**
 * @file wrist_motor_only_monitor.cpp
 * @brief Wrist Only Motor Angle Monitor (No Arm Connection)
 *
 * Simple monitoring for wrist motors (m7-m8) angles without arm connection.
 * Only reads wrist component data from motors 7 and 8.
 *
 * @author IC_CAN Team
 * @date 2025-10-28
 */

#include <iostream>
#include <iomanip>
#include <fstream>
#include <signal.h>
#include <chrono>
#include <thread>
#include <atomic>
#include <cmath>
#include "ic_can/core/wrist_component.hpp"

// Global flag for graceful shutdown
std::atomic<bool> keep_running(true);

// Signal handler for Ctrl+C
void signal_handler(int signal) {
    if (signal == SIGINT) {
        std::cout << "\n🛑 Received interrupt signal, shutting down gracefully..." << std::endl;
        keep_running = false;
    }
}

void print_usage(const char* program_name) {
    std::cout << "=== IC_CAN Wrist Only Motor Monitor ===" << std::endl;
    std::cout << "Simple angle monitoring for wrist motors (m7-m8) - No arm connection required" << std::endl;
    std::cout << std::endl;
    std::cout << "Usage: " << program_name << " [options]" << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  -f <freq>     Update frequency in Hz (default: 20)" << std::endl;
    std::cout << "  -l            Enable data logging to CSV file" << std::endl;
    std::cout << "  -t <seconds>  Run for specified time, then exit" << std::endl;
    std::cout << "  -h            Show this help message" << std::endl;
    std::cout << std::endl;
    std::cout << "Example:" << std::endl;
    std::cout << "  " << program_name << " -f 30 -l     # Monitor at 30Hz with logging" << std::endl;
    std::cout << "  " << program_name << " -t 60        # Monitor for 60 seconds only" << std::endl;
}

void print_header() {
    std::cout << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Motor | Position (rad) | Position (deg)" << std::endl;
    std::cout << "------|---------------|----------------" << std::endl;
}

void print_motor_data(int motor_id, double pos_rad, const std::string& motor_name) {
    double pos_deg = pos_rad * 180.0 / M_PI;
    std::cout << std::fixed << std::setprecision(3);
    std::cout << std::setw(4) << motor_id << "   | "
              << std::setw(11) << pos_rad << " | "
              << std::setw(12) << pos_deg << "  "
              << "(" << motor_name << ")" << std::endl;
}

void log_to_csv(std::ofstream& csv_file, double timestamp,
                double m7_pos, double m8_pos) {
    if (csv_file.is_open()) {
        csv_file << std::fixed << std::setprecision(6);
        csv_file << timestamp << ","
                 << m7_pos << "," << m8_pos << ","
                 << (m7_pos * 180.0 / M_PI) << "," << (m8_pos * 180.0 / M_PI)
                 << std::endl;
    }
}

int main(int argc, char* argv[]) {
    // Default parameters
    double frequency = 20.0;  // Hz
    bool enable_logging = false;
    double duration = -1.0;   // seconds, -1 means continuous

    // Parse command line arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "-h" || arg == "--help") {
            print_usage(argv[0]);
            return 0;
        } else if (arg == "-f" && i + 1 < argc) {
            frequency = std::stod(argv[++i]);
        } else if (arg == "-l") {
            enable_logging = true;
        } else if (arg == "-t" && i + 1 < argc) {
            duration = std::stod(argv[++i]);
        }
    }

    // Validate frequency
    if (frequency <= 0 || frequency > 1000) {
        std::cerr << "❌ Invalid frequency: " << frequency << " Hz. Must be between 0 and 1000 Hz." << std::endl;
        return 1;
    }

    // Set up signal handler
    signal(SIGINT, signal_handler);

    // Open CSV file for logging if enabled
    std::ofstream csv_file;
    if (enable_logging) {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << "wrist_motor_only_data_" << std::put_time(std::localtime(&time_t), "%Y%m%d_%H%M%S") << ".csv";
        csv_file.open(ss.str());
        if (csv_file.is_open()) {
            csv_file << "timestamp,m7_pos_rad,m8_pos_rad,m7_pos_deg,m8_pos_deg" << std::endl;
            std::cout << "📝 Logging enabled: " << ss.str() << std::endl;
        } else {
            std::cerr << "⚠️  Failed to open CSV file for logging" << std::endl;
            enable_logging = false;
        }
    }

    // Print configuration
    std::cout << "=== IC_CAN Wrist Only Motor Monitor ===" << std::endl;
    std::cout << "Simple angle monitoring for wrist motors (m7-m8) - No arm connection required" << std::endl;
    std::cout << "📊 Monitoring frequency: " << frequency << " Hz" << std::endl;
    std::cout << "📝 Logging: " << (enable_logging ? "Enabled" : "Disabled") << std::endl;
    if (duration > 0) {
        std::cout << "⏱️  Duration: " << duration << " seconds" << std::endl;
    } else {
        std::cout << "⏱️  Duration: Continuous (Ctrl+C to stop)" << std::endl;
    }
    std::cout << std::endl;

    try {
        std::cout << "🔧 Creating wrist component (standalone)..." << std::endl;

        // Create wrist component directly without IC_CAN system
        ic_can::WristComponent wrist;

        std::cout << "✅ Wrist component created successfully" << std::endl;
        std::cout << "🚀 Starting wrist motor angle monitoring..." << std::endl;
        std::cout << "Press Ctrl+C to stop" << std::endl;

        // Monitoring loop
        auto start_time = std::chrono::steady_clock::now();
        auto last_update = start_time;
        const auto update_interval = std::chrono::duration<double>(1.0 / frequency);

        print_header();

        while (keep_running) {
            auto current_time = std::chrono::steady_clock::now();

            // Check duration limit
            if (duration > 0) {
                auto elapsed = std::chrono::duration<double>(current_time - start_time).count();
                if (elapsed >= duration) {
                    std::cout << "⏰ Time limit reached, stopping monitoring" << std::endl;
                    break;
                }
            }

            // Update at specified frequency
            if (current_time - last_update >= update_interval) {
                // Get wrist motor positions directly from component
                auto positions = wrist.get_positions();

                if (positions.size() >= 2) {
                    double m7_pos = positions[0];  // Pitch
                    double m8_pos = positions[1];  // Roll

                    // Calculate timestamp
                    double timestamp = std::chrono::duration<double>(
                        current_time - start_time).count();

                    // Print motor data
                    print_motor_data(7, m7_pos, "Pitch");
                    print_motor_data(8, m8_pos, "Roll");

                    // Log to CSV if enabled
                    if (enable_logging) {
                        log_to_csv(csv_file, timestamp, m7_pos, m8_pos);
                    }

                    // Calculate wrist pose summary
                    double pitch_deg = m7_pos * 180.0 / M_PI;
                    double roll_deg = m8_pos * 180.0 / M_PI;

                    std::cout << "------|---------------|----------------" << std::endl;
                    std::cout << "Summary: Pitch=" << std::fixed << std::setprecision(1)
                              << pitch_deg << "°, Roll=" << roll_deg << "°" << std::endl;
                    std::cout << "Time: " << std::setprecision(1) << timestamp << "s" << std::endl;
                    std::cout << "========================================" << std::endl;
                } else {
                    std::cout << "⚠️  Wrist component not connected to motors" << std::endl;
                    std::cout << "    Motors 7-8 not found - returning default values" << std::endl;

                    // Show default values when motors not connected
                    double m7_pos = 0.0;  // Default pitch
                    double m8_pos = 0.0;  // Default roll
                    double timestamp = std::chrono::duration<double>(
                        current_time - start_time).count();

                    print_motor_data(7, m7_pos, "Pitch (no motor)");
                    print_motor_data(8, m8_pos, "Roll (no motor)");

                    if (enable_logging) {
                        log_to_csv(csv_file, timestamp, m7_pos, m8_pos);
                    }

                    std::cout << "------|---------------|----------------" << std::endl;
                    std::cout << "Summary: Pitch=0.0°, Roll=0.0° (motors not connected)" << std::endl;
                    std::cout << "Time: " << std::setprecision(1) << timestamp << "s" << std::endl;
                    std::cout << "========================================" << std::endl;
                }

                last_update = current_time;
            }

            // Small sleep to prevent CPU overload
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        // Cleanup
        if (enable_logging && csv_file.is_open()) {
            csv_file.close();
            std::cout << "📁 CSV file saved successfully" << std::endl;
        }

        std::cout << "✅ Monitoring stopped successfully" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "❌ Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}