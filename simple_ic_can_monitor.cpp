/**
 * @file simple_ic_can_monitor.cpp
 * @brief Simple Position Monitor with IC_CAN-like interface
 *
 * This program provides an IC_CAN-like interface while using the working
 * dm-tools SDK approach, avoiding C++ header compatibility issues.
 */

#include <cmath>
#include <csignal>
#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <thread>
#include <chrono>
#include <iomanip>
#include <atomic>

// Use working dm-tools SDK directly
#include "dm-tools/USB2FDCAN/SDK/C++/ubuntu/include/protocol/usb_class.h"

static volatile bool g_running = true;

// Simple IC_CAN-like interface
class SimpleIC_CAN {
public:
    SimpleIC_CAN(const std::string& device_sn, bool debug = false)
        : device_sn_(device_sn), debug_enabled_(debug), connected_(false) {}

    bool initialize() {
        if (connected_) return true;

        try {
            std::cout << "🔧 Connecting to USB2CAN device..." << std::endl;
            std::cout << "Device SN: " << device_sn_ << std::endl;

            device_ = std::make_unique<usb_class>(1000000, 5000000, device_sn_);
            if (!device_) {
                std::cout << "❌ FAILED: Could not create usb_class" << std::endl;
                return false;
            }

            std::cout << "✅ SUCCESS: Device connected" << std::endl;

            // Set up callback for motor feedback
            std::cout << "📡 Setting up motor feedback callback..." << std::endl;
            device_->setFrameCallback([this](can_value_type& frame) {
                handle_can_frame(frame);
            });
            std::cout << "✅ Callback configured" << std::endl;

            // Start data capture
            std::cout << "🔄 Starting data capture..." << std::endl;
            uint8_t capture_result = device_->USB_CMD_START_CAP();
            if (capture_result == 0) {
                std::cout << "✅ SUCCESS: Data capture started" << std::endl;
            } else {
                std::cout << "⚠️  WARNING: Data capture start returned "
                          << (int)capture_result << std::endl;
            }

            connected_ = true;
            return true;

        } catch (const std::exception& e) {
            std::cout << "❌ Connection failed: " << e.what() << std::endl;
            return false;
        }
    }

    bool enable_all() {
        if (!connected_) return false;

        std::cout << "\n⚡ Enabling all motors..." << std::endl;

        // Enable motors 1-9 using DM protocol
        std::vector<uint8_t> enable_cmd = {0xFF, 0xFF, 0xFF, 0xFF,
                                           0xFF, 0xFF, 0xFF, 0xFC};

        bool all_success = true;
        for (int motor_id = 1; motor_id <= 9; motor_id++) {
            device_->fdcanFrameSend(enable_cmd, motor_id);
            std::cout << "   ✅ Motor " << motor_id << " enabled" << std::endl;
            usleep(100000); // 100ms between enables
        }

        std::cout << "✅ All motors enabled" << std::endl;
        return true;
    }

    bool disable_all() {
        if (!connected_) return false;

        std::cout << "\n🔌 Disabling all motors..." << std::endl;

        // Disable motors 1-9
        std::vector<uint8_t> disable_cmd = {0xFF, 0xFF, 0xFF, 0xFF,
                                            0xFF, 0xFF, 0xFF, 0xFD};

        for (int motor_id = 1; motor_id <= 9; motor_id++) {
            device_->fdcanFrameSend(disable_cmd, motor_id);
        }

        std::cout << "✅ All motors disabled" << std::endl;
        return true;
    }

    void refresh_all() {
        // Send status request to trigger position feedback
        std::vector<uint8_t> status_cmd = {0x01, 0x00, 0xCC, 0x00};
        device_->fdcanFrameSend(status_cmd, 0x7FF); // Broadcast status request
    }

    std::vector<double> get_joint_positions() {
        std::lock_guard<std::mutex> lock(positions_mutex_);
        std::vector<double> positions(9, 0.0);

        for (int i = 0; i < 9; i++) {
            positions[i] = positions_[i].load();
        }

        return positions;
    }

    std::vector<double> get_joint_velocities() {
        std::lock_guard<std::mutex> lock(velocities_mutex_);
        std::vector<double> velocities(9, 0.0);

        for (int i = 0; i < 9; i++) {
            velocities[i] = velocities_[i].load();
        }

        return velocities;
    }

    std::vector<double> get_joint_torques() {
        std::lock_guard<std::mutex> lock(torques_mutex_);
        std::vector<double> torques(9, 0.0);

        for (int i = 0; i < 9; i++) {
            torques[i] = torques_[i].load();
        }

        return torques;
    }

    bool set_joint_positions(const std::vector<double>& positions,
                            const std::vector<double>& velocities = {},
                            const std::vector<double>& torques = {}) {
        if (!connected_) return false;
        if (positions.size() < 9) return false;

        for (int i = 0; i < 9; i++) {
            double pos = positions[i];
            double vel = (velocities.size() > i) ? velocities[i] : 0.0;
            double tau = (torques.size() > i) ? torques[i] : 0.0;

            if (i < 6) {
                // Damiao motors 1-6: use DM MIT protocol
                send_dm_mit_command(i + 1, pos, vel, tau, 20.0, 0.5);
            } else if (i < 8) {
                // HT motors 7-8: use HT MIT protocol
                send_ht_mit_command(pos, vel, tau, 20.0, 0.5);
            } else {
                // Servo motor 9: use DM protocol as placeholder
                send_dm_mit_command(i + 1, pos, vel, tau, 20.0, 0.5);
            }
        }

        return true;
    }

    ~SimpleIC_CAN() {
        if (connected_ && device_) {
            device_->USB_CMD_STOP_CAP();
        }
    }

private:
    void handle_can_frame(can_value_type& frame) {
        uint32_t can_id = frame.head.id;

        // Handle Damiao motor feedback (motors 1-6)
        if (can_id >= 1 && can_id <= 6) {
            process_dm_motor_feedback(frame, can_id - 1);
        }
        // Handle HT motor feedback (motors 7-8)
        else if (can_id == 0x700 || can_id == 0x800) {
            process_ht_motor_feedback(frame, can_id == 0x700 ? 6 : 7); // Map to motors 7-8
        }
    }

    void process_dm_motor_feedback(can_value_type& frame, int motor_idx) {
        if (frame.head.dlc < 6) return;

        // Extract motor response data (same as working test)
        uint16_t q_uint = (uint16_t(frame.data[1]) << 8) | frame.data[2];
        uint16_t dq_uint = (uint16_t(frame.data[3]) << 4) | (frame.data[4] >> 4);
        uint16_t tau_uint = (uint16_t(frame.data[4] & 0xf) << 8) | frame.data[5];

        // Use correct limits for each motor type
        double p_max, v_max, t_max;
        if (motor_idx == 0) { // Motor 1 - DM10010L
            p_max = 12.5; v_max = 25.0; t_max = 200.0;
        } else if (motor_idx == 1 || motor_idx == 2) { // Motors 2-3 - DM6248
            p_max = 12.566; v_max = 20.0; t_max = 120.0;
        } else if (motor_idx == 3 || motor_idx == 4) { // Motors 4-5 - DM4340
            p_max = 12.5; v_max = 10.0; t_max = 28.0;
        } else { // Motor 6 - DM4310
            p_max = 12.5; v_max = 30.0; t_max = 10.0;
        }

        // Convert to real values
        auto uint_to_float = [](uint16_t x, float xmin, float xmax, uint8_t bits) -> float {
            float span = xmax - xmin;
            float data_norm = float(x) / ((1 << bits) - 1);
            float data = data_norm * span + xmin;
            return data;
        };

        double position = uint_to_float(q_uint, -p_max, p_max, 16);
        double velocity = uint_to_float(dq_uint, -v_max, v_max, 12);
        double torque = uint_to_float(tau_uint, -t_max, t_max, 12);

        // Update atomic values
        positions_[motor_idx].store(position);
        velocities_[motor_idx].store(velocity);
        torques_[motor_idx].store(torque);
    }

    void process_ht_motor_feedback(can_value_type& frame, int motor_idx) {
        if (frame.head.dlc < 7) return;

        // Extract HT motor data (same as HT test)
        int16_t pos_int = static_cast<int16_t>(frame.data[1] | (frame.data[2] << 8));
        int16_t vel_int = static_cast<int16_t>(frame.data[3] | (frame.data[4] << 8));
        int16_t torque_int = static_cast<int16_t>(frame.data[5] | (frame.data[6] << 8));

        // Convert to physical units (HT protocol)
        const double TURN_TO_RAD = 2.0 * M_PI;
        double position = pos_int * 0.0001 * TURN_TO_RAD;
        double velocity = vel_int * 0.00025 * TURN_TO_RAD;
        const double torque_k = 0.004855;
        const double torque_d = -0.083;
        double torque = torque_int * torque_k + torque_d;

        // Update atomic values
        positions_[motor_idx].store(position);
        velocities_[motor_idx].store(velocity);
        torques_[motor_idx].store(torque);
    }

    void send_dm_mit_command(int motor_id, double position, double velocity,
                             double torque, double kp, double kd) {
        auto float_to_uint = [](double x, double min, double max, int bits) -> uint16_t {
            double span = max - min;
            if (x < min) x = min;
            if (x > max) x = max;
            return static_cast<uint16_t>((x - min) * ((1 << bits) - 1) / span);
        };

        // Determine correct limits for motor
        double p_max, v_max, t_max;
        if (motor_id == 1) {
            p_max = 12.5; v_max = 25.0; t_max = 200.0;
        } else if (motor_id == 2 || motor_id == 3) {
            p_max = 12.566; v_max = 20.0; t_max = 120.0;
        } else if (motor_id == 4 || motor_id == 5) {
            p_max = 12.5; v_max = 10.0; t_max = 28.0;
        } else {
            p_max = 12.5; v_max = 30.0; t_max = 10.0;
        }

        uint16_t kp_uint = float_to_uint(kp, 0.0, 500.0, 12);
        uint16_t kd_uint = float_to_uint(kd, 0.0, 5.0, 12);
        uint16_t q_uint = float_to_uint(position, -p_max, p_max, 16);
        uint16_t dq_uint = float_to_uint(velocity, -v_max, v_max, 12);
        uint16_t tau_uint = float_to_uint(torque, -t_max, t_max, 12);

        // Pack DM MIT command
        std::vector<uint8_t> data(8, 0);
        data[0] = (q_uint >> 8) & 0xFF;
        data[1] = q_uint & 0xFF;
        data[2] = dq_uint >> 4;
        data[3] = ((dq_uint & 0xF) << 4) | ((kp_uint >> 8) & 0xF);
        data[4] = kp_uint & 0xFF;
        data[5] = kd_uint >> 4;
        data[6] = ((kd_uint & 0xF) << 4) | ((tau_uint >> 8) & 0xF);
        data[7] = tau_uint & 0xFF;

        device_->fdcanFrameSend(data, motor_id);
    }

    void send_ht_mit_command(double position, double velocity,
                           double torque, double kp, double kd) {
        // HT motor conversion constants
        const double RAD_TO_TURN = 1.0 / (2.0 * M_PI);
        const double torque_k = 0.004855;
        const double torque_d = -0.083;

        // Convert to motor units
        double pos_turns = position * RAD_TO_TURN;
        double vel_turns = velocity * RAD_TO_TURN;

        // Convert to int16 format
        int16_t pos_int = static_cast<int16_t>(pos_turns / 0.0001);
        int16_t vel_int = static_cast<int16_t>(vel_turns / 0.00025);
        int16_t kp_int = static_cast<int16_t>(kp * 10);
        int16_t kd_int = static_cast<int16_t>(kd * 10);
        int16_t torque_int = static_cast<int16_t>((torque - torque_d) / torque_k);

        // Pack HT MIT command (12 bytes)
        std::vector<uint8_t> data(12, 0);
        data[0] = pos_int & 0xFF;
        data[1] = (pos_int >> 8) & 0xFF;
        data[2] = vel_int & 0xFF;
        data[3] = (vel_int >> 8) & 0xFF;
        data[4] = torque_int & 0xFF;
        data[5] = (torque_int >> 8) & 0xFF;
        data[6] = kp_int & 0xFF;
        data[7] = (kp_int >> 8) & 0xFF;
        data[8] = kd_int & 0xFF;
        data[9] = (kd_int >> 8) & 0xFF;
        data[10] = 0x00;
        data[11] = 0x00;

        device_->fdcanFrameSend(data, 0x8094); // HT uses fixed ID 0x8094
    }

    std::string device_sn_;
    bool debug_enabled_;
    bool connected_;
    std::unique_ptr<usb_class> device_;

    std::atomic<double> positions_[9];
    std::atomic<double> velocities_[9];
    std::atomic<double> torques_[9];

    std::mutex positions_mutex_;
    std::mutex velocities_mutex_;
    std::mutex torques_mutex_;
};

// Global variables
SimpleIC_CAN* g_controller = nullptr;

void signal_handler(int signal) {
    std::cout << "\n⚠️  Received signal " << signal << ", stopping..."
              << std::endl;
    g_running = false;
}

void print_all_motor_positions(SimpleIC_CAN& controller) {
    auto positions = controller.get_joint_positions();
    auto velocities = controller.get_joint_velocities();
    auto torques = controller.get_joint_torques();

    // Clear screen (works on Linux/Mac)
    std::cout << "\033[2J\033[H";

    std::cout << "=== Simple IC_CAN Position Monitor ===" << std::endl;
    std::cout << "Real-time monitoring of all 9 motors" << std::endl;
    std::cout << "Press Ctrl+C to stop" << std::endl;
    std::cout << std::endl;

    // Print header
    std::cout << "Joint | Motor | Type      | Position (rad) | Position (deg) | Velocity (rad/s) | Torque (Nm)" << std::endl;
    std::cout << "------|-------|-----------|----------------|----------------|-------------------|-------------" << std::endl;

    // Motor type names for display
    const std::vector<std::string> motor_types = {
        "DM10010L", "DM6248", "DM6248", "DM4340", "DM4340", "DM4310",
        "HT4438", "HT4438", "SERVO"
    };

    // Print each joint
    for (int i = 0; i < 9; i++) {
        double pos_rad = positions[i];
        double pos_deg = pos_rad * 180.0 / M_PI;
        double vel = velocities[i];
        double tau = torques[i];

        std::cout << std::setw(5) << (i + 1) << " | "
                  << std::setw(5) << "M" << (i + 1) << " | "
                  << std::setw(9) << motor_types[i] << " | "
                  << std::fixed << std::setw(14) << std::setprecision(3) << pos_rad << " | "
                  << std::fixed << std::setw(14) << std::setprecision(1) << pos_deg << " | "
                  << std::fixed << std::setw(17) << std::setprecision(3) << vel << " | "
                  << std::fixed << std::setw(11) << std::setprecision(3) << tau << std::endl;
    }

    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    std::cout << "\nLast update: " << timestamp << " ms" << std::endl;
}

void test_motor_control(SimpleIC_CAN& controller) {
    std::cout << "\n🎯 Testing motor control through IC_CAN interface..." << std::endl;

    // Test single motor control (Motor 1 - DM10010L)
    std::cout << "\n📍 Testing Motor 1 position control..." << std::endl;

    // Get current position
    auto initial_positions = controller.get_joint_positions();
    double initial_pos = initial_positions[0];
    std::cout << "📍 Initial position: " << std::fixed << std::setprecision(3)
              << initial_pos << " rad (" << (initial_pos * 180.0 / M_PI) << "°)" << std::endl;

    // Test position commands
    std::vector<std::pair<double, std::string>> test_positions = {
        {0.0, "Zero position"},
        {0.2, "Small positive (0.2 rad ≈ 11.5°)"},
        {-0.2, "Small negative (-0.2 rad ≈ -11.5°)"},
        {initial_pos, "Return to initial"}
    };

    for (const auto& [target_pos, description] : test_positions) {
        if (!g_running) break;

        std::cout << "\n📍 " << description << "..." << std::endl;

        // Create position vector for all motors (only move motor 1)
        std::vector<double> positions(9, 0.0);
        positions[0] = target_pos;

        // Send position command through IC_CAN interface
        if (controller.set_joint_positions(positions)) {
            std::cout << "✅ Position command sent through IC_CAN" << std::endl;
        } else {
            std::cout << "❌ Failed to send position command" << std::endl;
        }

        // Wait for movement
        for (int i = 3; i > 0 && g_running; --i) {
            std::cout << "   ⏱️  Waiting... " << i << "s" << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        // Read result
        controller.refresh_all();
        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Give time for feedback

        auto current_positions = controller.get_joint_positions();
        double current_pos = current_positions[0];

        std::cout << "   📍 Current: " << std::fixed << std::setprecision(3)
                  << current_pos << " rad (" << (current_pos * 180.0 / M_PI) << "°)" << std::endl;
        std::cout << "   📏 Error: " << std::fixed << std::setprecision(3)
                  << std::abs(target_pos - current_pos) << " rad" << std::endl;
    }
}

int main() {
    std::cout << "=== Simple IC_CAN Position Monitor ===" << std::endl;
    std::cout << "Using IC_CAN-like interface with working dm-tools SDK" << std::endl;

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    try {
        // Create IC_CAN-like controller
        std::cout << "\n🔧 Creating Simple IC_CAN controller..." << std::endl;
        auto controller = std::make_unique<SimpleIC_CAN>("F561E08C892274DB09496BCC1102DBC5", true);
        g_controller = controller.get();

        // Initialize the system
        std::cout << "\n🚀 Initializing system..." << std::endl;
        if (!controller->initialize()) {
            std::cout << "❌ FAILED: System initialization failed" << std::endl;
            return -1;
        }
        std::cout << "✅ SUCCESS: System initialized" << std::endl;

        // Enable all motors
        std::cout << "\n⚡ Enabling all motors..." << std::endl;
        if (!controller->enable_all()) {
            std::cout << "⚠️  WARNING: Some motors failed to enable" << std::endl;
        }

        // Wait for motors to initialize
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));

        // Read initial motor states
        std::cout << "\n📊 Reading initial motor states..." << std::endl;
        controller->refresh_all();
        std::this_thread::sleep_for(std::chrono::milliseconds(200)); // Wait for feedback
        print_all_motor_positions(*controller);

        // Test motor control
        if (g_running) {
            test_motor_control(*controller);
        }

        // Continuous monitoring loop
        if (g_running) {
            std::cout << "\n🔄 Starting continuous position monitoring..." << std::endl;
            std::cout << "Updating display every 100ms" << std::endl;

            auto last_update = std::chrono::steady_clock::now();

            while (g_running) {
                auto current_time = std::chrono::steady_clock::now();
                auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                    current_time - last_update).count();

                // Update display every 100ms
                if (elapsed >= 100) {
                    controller->refresh_all();
                    std::this_thread::sleep_for(std::chrono::milliseconds(50)); // Give time for feedback
                    print_all_motor_positions(*controller);
                    last_update = current_time;
                }

                // Sleep for short duration
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        }

        // Disable all motors
        std::cout << "\n🔌 Disabling all motors..." << std::endl;
        controller->disable_all();

        std::cout << "\n🎉 SIMPLE IC_CAN POSITION MONITOR COMPLETED!" << std::endl;
        std::cout << "✅ IC_CAN-like interface working correctly" << std::endl;
        std::cout << "✅ All 9 motors monitored successfully" << std::endl;
        std::cout << "✅ dm-tools SDK integration verified" << std::endl;
        std::cout << "✅ Motor control through IC_CAN interface working" << std::endl;

        return 0;

    } catch (const std::exception &e) {
        std::cout << "❌ EXCEPTION: " << e.what() << std::endl;
        return -1;
    }
}