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
 * @file trajectory_executor.cpp
 * @brief JSON Trajectory Executor Tool
 *
 * Tool to load and execute trajectories from JSON files:
 * - Load trajectory data from JSON files
 * - Execute at specified frequency
 * - Monitor execution progress
 * - Safety checks and error handling
 */

#include <chrono>
#include <csignal>
#include <fstream>
#include <ic_can/core/ic_can.hpp>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

// Simple JSON parser for trajectory files
class SimpleJSONParser {
public:
  static bool parse_trajectory(const std::string &json_file, double &frequency,
                              std::vector<double> &time_points,
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
               (content[start] == ' ' || content[start] == '\t' || content[start] == '\n')) {
          start++;
        }
        size_t end = start;
        while (end < content.size() &&
               (content[end] >= '0' && content[end] <= '9' || content[end] == '.')) {
          end++;
        }
        try {
          frequency = std::stod(content.substr(start, end - start));
        } catch (...) {
          return false;
        }
      }
    }

    // Parse time array
    size_t time_pos = content.find("\"time\"");
    if (time_pos != std::string::npos) {
      size_t array_start = content.find('[', time_pos);
      if (array_start != std::string::npos) {
        size_t array_end = find_matching_bracket(content, array_start, '[', ']');
        if (array_end != std::string::npos) {
          std::string time_str = content.substr(array_start + 1, array_end - array_start - 1);
          time_points = parse_number_array(time_str);
        }
      }
    }

    // Parse positions array
    size_t pos_pos = content.find("\"positions\"");
    if (pos_pos != std::string::npos) {
      size_t array_start = content.find('[', pos_pos);
      if (array_start != std::string::npos) {
        size_t array_end = find_matching_bracket(content, array_start, '[', ']');
        if (array_end != std::string::npos) {
          std::string pos_str = content.substr(array_start + 1, array_end - array_start - 1);
          positions = parse_positions_array(pos_str);
        }
      }
    }

    return !time_points.empty() && !positions.empty();
  }

private:
  static size_t find_matching_bracket(const std::string &str, size_t start, char open, char close) {
    int count = 1;
    for (size_t i = start + 1; i < str.size(); i++) {
      if (str[i] == open) {
        count++;
      } else if (str[i] == close) {
        count--;
        if (count == 0) {
          return i;
        }
      }
    }
    return std::string::npos;
  }

  static std::vector<double> parse_number_array(const std::string &str) {
    std::vector<double> result;
    std::string num_str;

    for (char c : str) {
      if ((c >= '0' && c <= '9') || c == '.' || c == '-' || c == '+' || c == 'e' || c == 'E') {
        num_str += c;
      } else if (!num_str.empty()) {
        try {
          result.push_back(std::stod(num_str));
        } catch (...) {
          // Skip invalid numbers
        }
        num_str.clear();
      }
    }

    if (!num_str.empty()) {
      try {
        result.push_back(std::stod(num_str));
      } catch (...) {
        // Skip invalid numbers
      }
    }

    return result;
  }

  static std::vector<std::vector<double>> parse_positions_array(const std::string &str) {
    std::vector<std::vector<double>> result;
    size_t start = 0;

    while (start < str.size()) {
      size_t inner_start = str.find('[', start);
      if (inner_start == std::string::npos) break;

      size_t inner_end = find_matching_bracket(str, inner_start, '[', ']');
      if (inner_end == std::string::npos) break;

      std::string inner_str = str.substr(inner_start + 1, inner_end - inner_start - 1);
      std::vector<double> pos = parse_number_array(inner_str);
      if (!pos.empty()) {
        result.push_back(pos);
      }

      start = inner_end + 1;
    }

    return result;
  }
};

static volatile bool g_running = true;

void signal_handler(int signal) {
  std::cout << "\n⚠️  Received signal " << signal << ", stopping trajectory..."
            << std::endl;
  g_running = false;
}

struct TrajectoryData {
  double frequency;
  std::vector<double> time_points;
  std::vector<std::vector<double>> positions;
  size_t total_points;
  double duration;

  bool load_from_json(const std::string &json_file) {
    std::cout << "📂 Loading trajectory from: " << json_file << std::endl;

    try {
      // Use our simple JSON parser
      if (!SimpleJSONParser::parse_trajectory(json_file, frequency, time_points,
                                            positions)) {
        std::cout << "❌ Failed to parse JSON file" << std::endl;
        return false;
      }

      std::cout << "📊 Trajectory frequency: " << frequency << " Hz" << std::endl;
      std::cout << "⏱️  Time points loaded: " << time_points.size() << std::endl;
      std::cout << "📍 Position waypoints loaded: " << positions.size() << std::endl;

      // Validate data consistency
      if (time_points.size() != positions.size()) {
        std::cout << "❌ ERROR: Time and position arrays have different sizes"
                  << std::endl;
        return false;
      }

      // Calculate trajectory properties
      total_points = positions.size();
      duration = time_points.back();
      std::cout << "⏱️  Trajectory duration: " << std::fixed << std::setprecision(2)
                << duration << " seconds" << std::endl;
      std::cout << "📍 Total trajectory points: " << total_points << std::endl;

      // Check position dimensions
      if (!positions.empty()) {
        size_t dof = positions[0].size();
        std::cout << "🦾 Degrees of freedom: " << dof << std::endl;

        // Validate all position vectors have same dimension
        for (size_t i = 0; i < positions.size(); i++) {
          if (positions[i].size() != dof) {
            std::cout << "❌ ERROR: Inconsistent DOF at point " << i << std::endl;
            return false;
          }
        }

        // Pad to 9 motors if needed (add zeros for missing motors)
        if (dof < 9) {
          std::cout << "📝 Padding from " << dof << " to 9 motors (adding zeros)"
                    << std::endl;
          for (auto &pos : positions) {
            pos.resize(9, 0.0);
          }
        }
      }

      return true;

    } catch (const std::exception &e) {
      std::cout << "❌ Error loading trajectory: " << e.what() << std::endl;
      return false;
    }
  }

  void print_info() const {
    std::cout << "\n📊 Trajectory Information:" << std::endl;
    std::cout << std::string(50, '=') << std::endl;
    std::cout << "Frequency: " << frequency << " Hz" << std::endl;
    std::cout << "Duration: " << std::fixed << std::setprecision(2) << duration
              << " seconds" << std::endl;
    std::cout << "Total Points: " << total_points << std::endl;
    std::cout << "DOF: " << (positions.empty() ? 0 : positions[0].size())
              << std::endl;

    if (!positions.empty()) {
      std::cout << "Start Position: ";
      for (size_t i = 0; i < positions[0].size() && i < 9; i++) {
        std::cout << std::fixed << std::setprecision(3) << positions[0][i] << " ";
      }
      std::cout << std::endl;

      std::cout << "End Position:   ";
      for (size_t i = 0; i < positions.back().size() && i < 9; i++) {
        std::cout << std::fixed << std::setprecision(3) << positions.back()[i]
                  << " ";
      }
      std::cout << std::endl;
    }
    std::cout << std::string(50, '=') << std::endl;
  }
};

void print_usage(const char *program_name) {
  std::cout << "Usage: " << program_name << " [options] <trajectory_file.json>"
            << std::endl;
  std::cout << "Options:" << std::endl;
  std::cout << "  -l             Enable logging" << std::endl;
  std::cout << "  -h             Show this help message" << std::endl;
  std::cout << "\nExample:" << std::endl;
  std::cout << "  " << program_name
            << " trajectory.json                 # Execute trajectory" << std::endl;
  std::cout << "  " << program_name
            << " -l trajectory.json              # Execute with logging" << std::endl;
}

int main(int argc, char *argv[]) {
  std::cout << "=== IC_CAN Trajectory Executor ===" << std::endl;
  std::cout << "Load and execute trajectories from JSON files" << std::endl;

  // Parse command line arguments
  bool enable_logging = false;
  std::string trajectory_file;

  for (int i = 1; i < argc; i++) {
    std::string arg = argv[i];

    if (arg == "-h" || arg == "--help") {
      print_usage(argv[0]);
      return 0;
    } else if (arg == "-l" || arg == "--log") {
      enable_logging = true;
    } else if (trajectory_file.empty()) {
      trajectory_file = arg;
    } else {
      std::cout << "❌ Unknown argument: " << arg << std::endl;
      print_usage(argv[0]);
      return -1;
    }
  }

  if (trajectory_file.empty()) {
    std::cout << "❌ ERROR: No trajectory file specified" << std::endl;
    print_usage(argv[0]);
    return -1;
  }

  signal(SIGINT, signal_handler);
  signal(SIGTERM, signal_handler);

  try {
    // Load trajectory data
    TrajectoryData trajectory;
    if (!trajectory.load_from_json(trajectory_file)) {
      return -1;
    }

    trajectory.print_info();

    // Create IC_CAN controller
    std::cout << "\n🔧 Initializing IC_CAN controller..." << std::endl;
    auto controller = std::make_unique<ic_can::IC_CAN>(
        "F561E08C892274DB09496BCC1102DBC5", true);

    if (!controller->initialize()) {
      std::cout << "❌ FAILED: System initialization failed" << std::endl;
      return -1;
    }
    std::cout << "✅ System initialized" << std::endl;

    // Enable motors
    if (!controller->enable_all()) {
      std::cout << "⚠️  WARNING: Some motors failed to enable" << std::endl;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    // Enable logging if requested
    if (enable_logging) {
      std::cout << "\n📝 Starting trajectory logging..." << std::endl;
      if (!controller->start_logging("./logs")) {
        std::cout << "❌ FAILED: Could not start logging" << std::endl;
      } else {
        std::cout << "✅ Logging enabled" << std::endl;
      }
    }

    // Enable frequency monitoring
    controller->enable_frequency_monitoring();

    // Read initial positions
    controller->refresh_all();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    auto initial_positions = controller->get_joint_positions();

    std::cout << "\n📍 Current motor positions: ";
    for (int i = 0; i < 6; i++) {
      std::cout << std::fixed << std::setprecision(3) << initial_positions[i] << " ";
    }
    std::cout << std::endl;

    std::cout << "📍 First trajectory position: ";
    for (int i = 0; i < 6; i++) {
      std::cout << std::fixed << std::setprecision(3)
                << trajectory.positions[0][i] << " ";
    }
    std::cout << std::endl;

    // Check if we need to move to starting position
    bool need_initial_move = false;
    for (int i = 0; i < 6; i++) {
      if (std::abs(initial_positions[i] - trajectory.positions[0][i]) > 0.01) {
        need_initial_move = true;
        break;
      }
    }

    if (need_initial_move) {
      std::cout << "\n🎯 Moving to starting position..." << std::endl;
      controller->set_target_positions_interpolated(trajectory.positions[0], 0.5);
      controller->start_control_loop(200.0); // 200Hz for initial move
      std::this_thread::sleep_for(std::chrono::seconds(3));
      controller->stop_control_loop();
      std::cout << "✅ Moved to starting position" << std::endl;
    }

    // Ask for confirmation
    std::cout << "\n🚀 Ready to execute trajectory!" << std::endl;
    std::cout << "   Duration: " << std::fixed << std::setprecision(1)
              << trajectory.duration << " seconds" << std::endl;
    std::cout << "   Frequency: " << trajectory.frequency << " Hz" << std::endl;
    std::cout << "\n🤔 Proceed with trajectory execution? (y/N): ";
    std::string confirm;
    std::getline(std::cin, confirm);

    if (confirm != "y" && confirm != "Y" && confirm != "yes" &&
        confirm != "YES") {
      std::cout << "❌ Cancelled trajectory execution" << std::endl;
      controller->disable_all();
      return 0;
    }

    std::cout << "\n🚀 Executing trajectory..." << std::endl;
    std::cout << "Press Ctrl+C to stop early" << std::endl;

    // Execute trajectory
    auto start_time = std::chrono::high_resolution_clock::now();
    size_t current_point = 0;

    while (g_running && current_point < trajectory.total_points) {
      auto loop_start = std::chrono::high_resolution_clock::now();

      // Send current position command
      controller->set_joint_positions(trajectory.positions[current_point], {}, {});

      current_point++;

      // Print progress every second
      auto elapsed = std::chrono::duration<double>(
                        std::chrono::high_resolution_clock::now() - start_time)
                        .count();
      static int last_progress_time = -1;
      int current_time_int = static_cast<int>(elapsed);
      if (current_time_int != last_progress_time) {
        double progress = (double)current_point / trajectory.total_points * 100.0;
        std::cout << "⏱️  Time: " << std::setw(3) << current_time_int << "s | "
                  << "Progress: " << std::fixed << std::setprecision(1)
                  << progress << "% | Point: " << current_point << "/"
                  << trajectory.total_points << std::endl;
        last_progress_time = current_time_int;
      }

      // Calculate sleep time to maintain trajectory frequency
      auto loop_end = std::chrono::high_resolution_clock::now();
      auto loop_duration = loop_end - loop_start;
      auto target_period = std::chrono::duration<double>(1.0 / trajectory.frequency);
      auto sleep_time = target_period - loop_duration;

      if (sleep_time.count() > 0) {
        std::this_thread::sleep_for(sleep_time);
      }
    }

    auto total_time = std::chrono::duration<double>(
                         std::chrono::high_resolution_clock::now() - start_time)
                         .count();

    std::cout << "\n✅ Trajectory execution completed!" << std::endl;
    std::cout << "   Total time: " << std::fixed << std::setprecision(2)
              << total_time << " seconds" << std::endl;
    std::cout << "   Points executed: " << current_point << "/"
              << trajectory.total_points << std::endl;

    if (current_point == trajectory.total_points) {
      std::cout << "   Status: SUCCESS" << std::endl;
    } else {
      std::cout << "   Status: STOPPED EARLY" << std::endl;
    }

    // Print final performance statistics
    std::cout << "\n📊 Final Performance Statistics:" << std::endl;
    controller->print_performance_stats();

    // Disable motors
    controller->disable_all();
    std::cout << "\n🎉 Trajectory executor finished!" << std::endl;

    return 0;

  } catch (const std::exception &e) {
    std::cout << "❌ EXCEPTION: " << e.what() << std::endl;
    return -1;
  }
}