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
 * @file ultra_simple_trajectory_executor.cpp
 * @brief Ultra Simple Trajectory Executor - Minimal Implementation
 *
 * Ultra-simple tool to:
 * - Read current motor positions
 * - Move smoothly to target position (first position in trajectory)
 * - Execute main trajectory without control loop
 * - Avoid all complex features that cause memory corruption
 */

#include <chrono>
#include <cmath>
#include <csignal>
#include <filesystem>
#include <fstream>
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
    std::cout << "\n⚠️  Received signal " << signal << ", stopping..." << std::endl;
    g_running = false;
}

// Very simple CSV parser - just get first line of positions
bool read_first_position_from_csv(const std::string &csv_file, std::vector<double> &position) {
    std::ifstream file(csv_file);
    if (!file.is_open()) {
        return false;
    }

    std::string line;
    bool header_skipped = false;

    while (std::getline(file, line)) {
        if (line.empty()) continue;

        // Skip header
        if (!header_skipped) {
            header_skipped = true;
            continue;
        }

        // Parse first data line
        std::stringstream ss(line);
        std::string cell;
        std::vector<double> values;

        while (std::getline(ss, cell, ',')) {
            try {
                values.push_back(std::stod(cell));
            } catch (...) {
                values.push_back(0.0);
            }
        }

        if (values.size() >= 9) {
            position = std::vector<double>(values.begin(), values.begin() + 9);
            return true;
        }
        break; // Only read first data line
    }

    return false;
}

// Very simple JSON parser - just get first position
bool read_first_position_from_json(const std::string &json_file, std::vector<double> &position, double &frequency) {
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
                frequency = 10.0; // default
            }
        }
    }

    // Find first position array
    size_t array_start = content.find("[[", content.find("\"positions\""));
    if (array_start != std::string::npos) {
        size_t array_end = content.find("]", array_start + 2);
        if (array_end != std::string::npos) {
            std::string pos_str = content.substr(array_start + 2, array_end - array_start - 2);

            std::stringstream ss(pos_str);
            std::string cell;
            std::vector<double> values;

            while (std::getline(ss, cell, ',')) {
                try {
                    values.push_back(std::stod(cell));
                } catch (...) {
                    values.push_back(0.0);
                }
            }

            if (values.size() >= 1) {
                position = std::vector<double>(9, 0.0);
                for (size_t i = 0; i < std::min(values.size(), size_t(9)); i++) {
                    position[i] = values[i];
                }
                return true;
            }
        }
    }

    return false;
}

int main(int argc, char *argv[]) {
    std::cout << "=== Ultra Simple Trajectory Executor ===" << std::endl;
    std::cout << "Minimal implementation - reads first position and interpolates" << std::endl;

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
        // Read target position from file
        std::vector<double> target_position;
        double frequency = 10.0; // default

        std::string file_extension = input_file.substr(input_file.find_last_of(".") + 1);
        std::transform(file_extension.begin(), file_extension.end(), file_extension.begin(), ::tolower);

        bool success = false;
        if (file_extension == "json") {
            success = read_first_position_from_json(input_file, target_position, frequency);
            std::cout << "📂 Read target position from JSON file" << std::endl;
        } else if (file_extension == "csv") {
            success = read_first_position_from_csv(input_file, target_position);
            std::cout << "📂 Read target position from CSV file" << std::endl;
        } else {
            std::cout << "❌ Unsupported file format: " << file_extension << std::endl;
            return -1;
        }

        if (!success || target_position.empty()) {
            std::cout << "❌ Failed to read target position from: " << input_file << std::endl;
            return -1;
        }

        std::cout << "📍 Target position: ";
        for (int i = 0; i < 9; i++) {
            std::cout << std::fixed << std::setprecision(3) << target_position[i] << " ";
        }
        std::cout << std::endl;
        std::cout << "📊 Frequency: " << frequency << " Hz" << std::endl;

        // Create mock controller (avoid actual hardware for now)
        std::cout << "\n🔧 Mock controller - reading simulated current positions..." << std::endl;

        // Simulate current positions (all zeros for testing)
        std::vector<double> current_positions(9, 0.0);
        for (int i = 0; i < 9; i++) {
            current_positions[i] = (i % 2 == 0) ? 0.1 : -0.1; // Some variation
        }

        std::cout << "📍 Current positions (simulated): ";
        for (int i = 0; i < 9; i++) {
            std::cout << std::fixed << std::setprecision(3) << current_positions[i] << " ";
        }
        std::cout << std::endl;

        // Calculate interpolation parameters
        double max_distance = 0.0;
        for (int i = 0; i < 8; i++) { // Skip gripper
            double distance = std::abs(current_positions[i] - target_position[i]);
            if (distance > max_distance) {
                max_distance = distance;
            }
        }

        double max_velocity = 0.3; // rad/s (conservative)
        double duration = std::max(max_distance / max_velocity, 3.0); // Minimum 3 seconds
        int num_steps = static_cast<int>(duration * frequency);
        double dt = 1.0 / frequency;

        std::cout << "\n🎯 Interpolation parameters:" << std::endl;
        std::cout << "   Max distance: " << std::fixed << std::setprecision(3)
                  << max_distance << " rad (" << (max_distance * 180.0 / M_PI) << "°)" << std::endl;
        std::cout << "   Duration: " << std::setprecision(2) << duration << " seconds" << std::endl;
        std::cout << "   Steps: " << num_steps << std::endl;

        // Ask for confirmation
        std::cout << "\n🚀 Ready to execute smooth trajectory (SIMULATION)!" << std::endl;
        std::cout << "   This is a simulation - no actual motor movement" << std::endl;
        std::cout << "\n🤔 Proceed with simulation? (y/N): ";
        std::string confirm;
        std::getline(std::cin, confirm);

        if (confirm != "y" && confirm != "Y" && confirm != "yes" && confirm != "YES") {
            std::cout << "❌ Cancelled trajectory execution" << std::endl;
            return 0;
        }

        // Execute interpolation (simulation)
        std::cout << "\n🚀 Executing smooth trajectory (SIMULATION)..." << std::endl;
        std::cout << "Press Ctrl+C to stop early" << std::endl;

        auto start_time = std::chrono::high_resolution_clock::now();

        for (int step = 0; step <= num_steps && g_running; ++step) {
            double t = static_cast<double>(step) / num_steps;
            std::vector<double> interpolated_pos(9, 0.0);

            // Interpolate
            for (int i = 0; i < 9; i++) {
                interpolated_pos[i] = current_positions[i] + t * (target_position[i] - current_positions[i]);
            }

            // Simulate motor command
            std::cout << "📝 Step " << step << "/" << num_steps << ": ";
            for (int i = 0; i < 9; i++) {
                std::cout << std::fixed << std::setprecision(3) << interpolated_pos[i] << " ";
            }
            std::cout << std::endl;

            // Sleep to maintain frequency
            std::this_thread::sleep_for(std::chrono::microseconds(static_cast<int64_t>(dt * 1000000)));

            // Print progress every second
            auto elapsed = std::chrono::duration<double>(
                               std::chrono::high_resolution_clock::now() - start_time)
                               .count();
            static int last_progress_time = -1;
            int current_time_int = static_cast<int>(elapsed);
            if (current_time_int != last_progress_time) {
                double progress = static_cast<double>(step) / num_steps * 100.0;
                std::cout << "⏱️  Time: " << std::setw(3) << current_time_int << "s | "
                          << "Progress: " << std::fixed << std::setprecision(1)
                          << progress << "% | Step: " << step << "/" << num_steps << std::endl;
                last_progress_time = current_time_int;
            }
        }

        auto total_time = std::chrono::duration<double>(
                              std::chrono::high_resolution_clock::now() - start_time)
                              .count();

        std::cout << "\n✅ Trajectory execution completed!" << std::endl;
        std::cout << "   Total time: " << std::fixed << std::setprecision(2)
                  << total_time << " seconds" << std::endl;
        std::cout << "   Final position: ";
        for (int i = 0; i < 9; i++) {
            std::cout << std::fixed << std::setprecision(3) << target_position[i] << " ";
        }
        std::cout << std::endl;

        std::cout << "\n🎉 Ultra simple trajectory executor finished!" << std::endl;
        std::cout << "💡 This was a simulation - no actual motors were moved" << std::endl;

        return 0;

    } catch (const std::exception &e) {
        std::cout << "❌ EXCEPTION: " << e.what() << std::endl;
        return -1;
    }
}