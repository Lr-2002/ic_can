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
 * @file simple_trajectory_executor_safe.cpp
 * @brief Simple Trajectory Executor - Safe Version (Avoids Gripper Issues)
 *
 * Simple tool to:
 * - Load trajectory data from JSON files
 * - Read current motor positions
 * - Interpolate from current state to the target initial pose
 * - Concatenate the two trajectories
 * - Execute smoothly WITHOUT gripper control (to avoid memory corruption)
 */

#include <chrono>
#include <cmath>
#include <csignal>
#include <filesystem>
#include <fstream>
#include <ic_can/core/ic_can.hpp>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static volatile bool g_running = true;

void signal_handler(int signal) {
    std::cout << "\n⚠️  Received signal " << signal << ", stopping trajectory..." << std::endl;
    g_running = false;
}

// Simple JSON parser for trajectory files
class SimpleJSONParser {
public:
    static bool parse_trajectory(const std::string &json_file, double &frequency,
                                std::vector<std::vector<double>> &positions) {
        std::ifstream file(json_file);
        if (!file.is_open()) {
            return false;
        }

        std::string content((std::istreambuf_iterator<char>(file)),
                            std::istreambuf_iterator<char>());

        // Parse frequency
        size_t freq_pos = content.find("\"frequency\"");
        if (freq_pos != std::string::npos) {
            size_t colon_pos = content.find(':', freq_pos);
            if (colon_pos != std::string::npos) {
                size_t start = colon_pos + 1;
                while (start < content.size() &&
                       (content[start] == ' ' || content[start] == '\t' ||
                        content[start] == '\n')) {
                    start++;
                }
                size_t end = start;
                while (end < content.size() &&
                       ((content[end] >= '0' && content[end] <= '9') ||
                        content[end] == '.')) {
                    end++;
                }
                try {
                    frequency = std::stod(content.substr(start, end - start));
                } catch (...) {
                    return false;
                }
            }
        }

        // Parse all positions (simplified parser)
        size_t pos_pos = content.find("\"positions\"");
        if (pos_pos != std::string::npos) {
            size_t array_start = content.find('[', pos_pos);
            if (array_start != std::string::npos) {
                size_t array_end = array_start + 1;
                int bracket_count = 1;

                for (size_t i = array_start + 1; i < content.size() && bracket_count > 0; i++) {
                    if (content[i] == '[') {
                        bracket_count++;
                    } else if (content[i] == ']') {
                        bracket_count--;
                    }
                    if (bracket_count == 1 && content[i] == ']') {
                        array_end = i;
                        break;
                    }
                }

                std::string pos_str = content.substr(array_start + 1, array_end - array_start - 1);

                // Parse each position array
                size_t start = 0;
                while (start < pos_str.size()) {
                    size_t inner_start = pos_str.find('[', start);
                    if (inner_start == std::string::npos) break;

                    size_t inner_end = pos_str.find(']', inner_start);
                    if (inner_end == std::string::npos) break;

                    std::string inner_str = pos_str.substr(inner_start + 1, inner_end - inner_start - 1);

                    std::vector<double> position;
                    std::string num_str;
                    for (char c : inner_str) {
                        if ((c >= '0' && c <= '9') || c == '.' || c == '-' || c == '+' ||
                            c == 'e' || c == 'E') {
                            num_str += c;
                        } else if (!num_str.empty()) {
                            try {
                                position.push_back(std::stod(num_str));
                            } catch (...) {
                                // Skip invalid numbers
                            }
                            num_str.clear();
                        }
                    }
                    if (!num_str.empty()) {
                        try {
                            position.push_back(std::stod(num_str));
                        } catch (...) {
                            // Skip invalid numbers
                        }
                    }

                    if (!position.empty()) {
                        // Ensure we have exactly 9 motors (pad or truncate)
                        position.resize(9, 0.0);
                        positions.push_back(position);
                    }

                    start = inner_end + 1;
                }
            }
        }

        return !positions.empty();
    }
};

// Simple CSV parser for trajectory files
class SimpleCSVParser {
public:
    static bool parse_trajectory(const std::string &csv_file, double &frequency,
                                std::vector<std::vector<double>> &positions) {
        std::ifstream file(csv_file);
        if (!file.is_open()) {
            return false;
        }

        std::string line;
        bool header_processed = false;

        while (std::getline(file, line)) {
            if (line.empty()) continue;

            // Skip header line
            if (!header_processed) {
                header_processed = true;
                continue;
            }

            std::vector<double> row_data;
            std::string cell;
            bool in_quotes = false;

            for (char c : line) {
                if (c == '"') {
                    in_quotes = !in_quotes;
                } else if (c == ',' && !in_quotes) {
                    if (!cell.empty()) {
                        try {
                            row_data.push_back(std::stod(cell));
                        } catch (...) {
                            row_data.push_back(0.0);
                        }
                    }
                    cell.clear();
                } else {
                    cell += c;
                }
            }

            // Add last cell
            if (!cell.empty()) {
                try {
                    row_data.push_back(std::stod(cell));
                } catch (...) {
                    row_data.push_back(0.0);
                }
            }

            // Extract position data (assume positions are columns 1-9 for motors 1-9)
            if (row_data.size() >= 9) {
                std::vector<double> position;
                for (int i = 0; i < 9; i++) {
                    position.push_back(row_data[i]);
                }
                positions.push_back(position);
            }
        }

        // Default frequency for CSV files (can be adjusted)
        frequency = 100.0; // 100 Hz default for CSV

        return !positions.empty();
    }
};

struct SimpleTrajectory {
    double frequency;
    std::vector<std::vector<double>> positions;

    void clear() {
        positions.clear();
    }

    void append_position(const std::vector<double> &pos) {
        positions.push_back(pos);
    }

    void append_trajectory(const SimpleTrajectory &other) {
        positions.insert(positions.end(), other.positions.begin(), other.positions.end());
    }

    size_t size() const {
        return positions.size();
    }

    bool empty() const {
        return positions.empty();
    }

    void print_info() const {
        std::cout << "📊 Trajectory Information:" << std::endl;
        std::cout << "   Frequency: " << frequency << " Hz" << std::endl;
        std::cout << "   Duration: " << std::fixed << std::setprecision(2)
                  << (positions.size() / frequency) << " seconds" << std::endl;
        std::cout << "   Total Points: " << positions.size() << std::endl;
        if (!positions.empty()) {
            std::cout << "   DOF: " << positions[0].size() << std::endl;
        }
    }
};

// Create interpolation trajectory from current to target position (SAFE - no gripper)
SimpleTrajectory create_interpolation_trajectory(const std::vector<double> &current_pos,
                                                 const std::vector<double> &target_pos,
                                                 double duration, double frequency) {
    SimpleTrajectory interp_traj;
    interp_traj.frequency = frequency;

    int num_points = static_cast<int>(duration * frequency);

    for (int i = 0; i <= num_points; i++) {
        double t = static_cast<double>(i) / num_points;
        std::vector<double> interpolated_pos(9, 0.0);

        for (int j = 0; j < 9; j++) {
            double current = (j < static_cast<int>(current_pos.size())) ? current_pos[j] : 0.0;
            double target = (j < static_cast<int>(target_pos.size())) ? target_pos[j] : 0.0;
            interpolated_pos[j] = current + t * (target - current);
        }

        interp_traj.append_position(interpolated_pos);
    }

    return interp_traj;
}

int main(int argc, char *argv[]) {
    std::cout << "=== Simple IC_CAN Trajectory Executor (SAFE) ===" << std::endl;
    std::cout << "Safe interpolation from current position to trajectory start" << std::endl;
    std::cout << "⚠️  NOTE: Gripper control disabled to avoid memory corruption" << std::endl;

    if (argc != 2) {
        std::cout << "Usage: " << argv[0] << " <trajectory_file.json|trajectory_file.csv>" << std::endl;
        return -1;
    }

    std::string input_file = argv[1];
    if (!std::filesystem::exists(input_file)) {
        std::cout << "❌ File not found: " << input_file << std::endl;
        return -1;
    }

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    try {
        // Detect file type and load trajectory to get target initial position
        SimpleTrajectory target_trajectory;
        std::string file_extension = input_file.substr(input_file.find_last_of(".") + 1);
        std::transform(file_extension.begin(), file_extension.end(), file_extension.begin(), ::tolower);

        bool load_success = false;
        if (file_extension == "json") {
            load_success = SimpleJSONParser::parse_trajectory(input_file, target_trajectory.frequency, target_trajectory.positions);
            std::cout << "📂 Loaded target trajectory from JSON file" << std::endl;
        } else if (file_extension == "csv") {
            load_success = SimpleCSVParser::parse_trajectory(input_file, target_trajectory.frequency, target_trajectory.positions);
            std::cout << "📂 Loaded target trajectory from CSV file" << std::endl;
        } else {
            std::cout << "❌ Unsupported file format: " << file_extension << std::endl;
            std::cout << "   Supported formats: .json, .csv" << std::endl;
            return -1;
        }

        if (!load_success) {
            std::cout << "❌ Failed to load trajectory: " << input_file << std::endl;
            return -1;
        }

        target_trajectory.print_info();

        if (target_trajectory.empty()) {
            std::cout << "❌ Trajectory is empty" << std::endl;
            return -1;
        }

        std::vector<double> target_initial_pos = target_trajectory.positions[0];
        std::cout << "📍 Target initial position: ";
        for (int i = 0; i < 9; i++) {
            std::cout << std::fixed << std::setprecision(3) << target_initial_pos[i] << " ";
        }
        std::cout << std::endl;

        // Initialize IC_CAN controller
        std::cout << "\n🔧 Initializing IC_CAN controller..." << std::endl;
        auto controller = std::make_unique<ic_can::IC_CAN>(
            "693D3DE86DF5940C8BC74A5B46A3CE2E", false);

        if (!controller->initialize()) {
            std::cout << "❌ FAILED: System initialization failed" << std::endl;
            return -1;
        }
        std::cout << "✅ System initialized" << std::endl;

        // Enable motors (skip gripper to avoid memory issues)
        if (!controller->enable_motors_1_to_8()) {
            std::cout << "⚠️  WARNING: Some motors failed to enable" << std::endl;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        // Set to EXECUTION_MODE
        controller->set_control_mode(ic_can::IC_CAN::ControlMode::EXECUTION_MODE);

        // Read current positions
        std::cout << "\n📍 Reading current motor positions..." << std::endl;
        controller->refresh_all();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        auto current_positions = controller->get_joint_positions();

        // Ensure we have 9 positions
        if (current_positions.size() < 9) {
            current_positions.resize(9, 0.0);
        }

        std::cout << "📍 Current positions: ";
        for (int i = 0; i < 9; i++) {
            std::cout << std::fixed << std::setprecision(3) << current_positions[i] << " ";
        }
        std::cout << std::endl;

        // Calculate maximum distance and duration
        double max_distance = 0.0;
        for (int i = 0; i < 8; i++) {  // Only motors 1-8, skip gripper (motor 9)
            double distance = std::abs(current_positions[i] - target_initial_pos[i]);
            if (distance > max_distance) {
                max_distance = distance;
            }
        }

        double max_velocity = 0.5; // rad/s for safety
        double interp_duration = std::max(max_distance / max_velocity, 2.0); // Minimum 2 seconds

        std::cout << "\n🎯 Interpolation parameters:" << std::endl;
        std::cout << "   Max distance (motors 1-8): " << std::fixed << std::setprecision(3)
                  << max_distance << " rad (" << (max_distance * 180.0 / M_PI) << "°)" << std::endl;
        std::cout << "   Max velocity: " << max_velocity << " rad/s" << std::endl;
        std::cout << "   Interpolation duration: " << std::setprecision(2)
                  << interp_duration << " seconds" << std::endl;

        // Create interpolation trajectory
        std::cout << "\n🔧 Creating interpolation trajectory..." << std::endl;
        SimpleTrajectory interpolation = create_interpolation_trajectory(
            current_positions, target_initial_pos, interp_duration, target_trajectory.frequency);

        std::cout << "✅ Created interpolation trajectory with " << interpolation.size() << " points" << std::endl;

        // Create combined trajectory
        SimpleTrajectory combined_trajectory;
        combined_trajectory.frequency = target_trajectory.frequency;

        // Append interpolation (except last point to avoid duplication)
        for (size_t i = 0; i < interpolation.size() - 1; i++) {
            combined_trajectory.append_position(interpolation.positions[i]);
        }

        // Append original trajectory
        combined_trajectory.append_trajectory(target_trajectory);

        std::cout << "\n📊 Combined trajectory:" << std::endl;
        combined_trajectory.print_info();

        // Ask for confirmation
        std::cout << "\n🚀 Ready to execute smooth trajectory!" << std::endl;
        std::cout << "   Total duration: " << std::fixed << std::setprecision(1)
                  << (combined_trajectory.size() / combined_trajectory.frequency) << " seconds" << std::endl;
        std::cout << "   Interpolation phase: " << std::setprecision(1)
                  << interp_duration << " seconds" << std::endl;
        std::cout << "   Main trajectory phase: " << std::setprecision(1)
                  << (target_trajectory.size() / target_trajectory.frequency) << " seconds" << std::endl;
        std::cout << "\n🤔 Proceed with execution? (y/N): ";
        std::string confirm;
        std::getline(std::cin, confirm);

        if (confirm != "y" && confirm != "Y" && confirm != "yes" && confirm != "YES") {
            std::cout << "❌ Cancelled trajectory execution" << std::endl;
            controller->disable_motors_1_to_8();
            return 0;
        }

        // Execute trajectory
        std::cout << "\n🚀 Executing smooth trajectory..." << std::endl;
        std::cout << "Press Ctrl+C to stop early" << std::endl;
        std::cout << "⚠️  Using safe mode - only motors 1-8 (gripper disabled)" << std::endl;

        auto start_time = std::chrono::high_resolution_clock::now();

        // Use simple sleep-based execution to avoid control loop issues
        for (size_t i = 0; i < combined_trajectory.size() && g_running; ++i) {
            // Safety check: ensure trajectory position has correct size
            if (i >= combined_trajectory.positions.size() || combined_trajectory.positions[i].size() < 9) {
                std::cout << "❌ ERROR: Invalid trajectory data at point " << i << std::endl;
                break;
            }

            try {
                // Send position command (motors 1-8 only, gripper position ignored)
                std::vector<double> arm_positions = combined_trajectory.positions[i];
                controller->set_joint_positions(arm_positions, {}, {});
            } catch (const std::exception& e) {
                std::cout << "❌ ERROR: Failed to send position command at point " << i
                          << ": " << e.what() << std::endl;
                // Continue execution even if individual commands fail
            }

            // Sleep to maintain frequency
            double period_us = 1000000.0 / combined_trajectory.frequency;
            std::this_thread::sleep_for(std::chrono::microseconds(static_cast<int64_t>(period_us)));

            // Print progress every second
            auto elapsed = std::chrono::duration<double>(
                               std::chrono::high_resolution_clock::now() - start_time)
                               .count();
            static int last_progress_time = -1;
            int current_time_int = static_cast<int>(elapsed);
            if (current_time_int != last_progress_time) {
                double progress = static_cast<double>(i) / combined_trajectory.size() * 100.0;
                std::cout << "⏱️  Time: " << std::setw(3) << current_time_int << "s | "
                          << "Progress: " << std::fixed << std::setprecision(1)
                          << progress << "% | Point: " << i << "/" << combined_trajectory.size() << std::endl;
                last_progress_time = current_time_int;
            }
        }

        auto total_time = std::chrono::duration<double>(
                              std::chrono::high_resolution_clock::now() - start_time)
                              .count();

        std::cout << "\n✅ Trajectory execution completed!" << std::endl;
        std::cout << "   Total time: " << std::fixed << std::setprecision(2)
                  << total_time << " seconds" << std::endl;

        // Disable motors
        controller->disable_motors_1_to_8();
        std::cout << "\n🎉 Simple trajectory executor finished!" << std::endl;

        return 0;

    } catch (const std::exception &e) {
        std::cout << "❌ EXCEPTION: " << e.what() << std::endl;
        return -1;
    }
}