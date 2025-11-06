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

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

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

    // Parse time array
    size_t time_pos = content.find("\"time\"");
    if (time_pos != std::string::npos) {
      size_t array_start = content.find('[', time_pos);
      if (array_start != std::string::npos) {
        size_t array_end =
            find_matching_bracket(content, array_start, '[', ']');
        if (array_end != std::string::npos) {
          std::string time_str =
              content.substr(array_start + 1, array_end - array_start - 1);
          time_points = parse_number_array(time_str);
        }
      }
    }

    // Parse positions array
    size_t pos_pos = content.find("\"positions\"");
    if (pos_pos != std::string::npos) {
      size_t array_start = content.find('[', pos_pos);
      if (array_start != std::string::npos) {
        size_t array_end =
            find_matching_bracket(content, array_start, '[', ']');
        if (array_end != std::string::npos) {
          std::string pos_str =
              content.substr(array_start + 1, array_end - array_start - 1);
          positions = parse_positions_array(pos_str);
        }
      }
    }

    return !time_points.empty() && !positions.empty();
  }

private:
  static size_t find_matching_bracket(const std::string &str, size_t start,
                                      char open, char close) {
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
      if ((c >= '0' && c <= '9') || c == '.' || c == '-' || c == '+' ||
          c == 'e' || c == 'E') {
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

  static std::vector<std::vector<double>>
  parse_positions_array(const std::string &str) {
    std::vector<std::vector<double>> result;
    size_t start = 0;

    while (start < str.size()) {
      size_t inner_start = str.find('[', start);
      if (inner_start == std::string::npos)
        break;

      size_t inner_end = find_matching_bracket(str, inner_start, '[', ']');
      if (inner_end == std::string::npos)
        break;

      std::string inner_str =
          str.substr(inner_start + 1, inner_end - inner_start - 1);
      std::vector<double> pos = parse_number_array(inner_str);
      if (!pos.empty()) {
        result.push_back(pos);
      }

      start = inner_end + 1;
    }

    return result;
  }
};

// CSV parser for motor states log files
class MotorStateCSVParser {
public:
  static bool parse_motor_states(const std::string &log_directory,
                                 double &frequency,
                                 std::vector<double> &time_points,
                                 std::vector<std::vector<double>> &positions) {

    // Find the motor_states.csv file in the log directory
    std::string motor_states_path;

    // Try to find the motor_states.csv file
    if (std::filesystem::exists(log_directory + "/motor_states.csv")) {
      motor_states_path = log_directory + "/motor_states.csv";
    } else {
      // Try to find in subdirectories (like ic_can_log_*)
      for (const auto &entry :
           std::filesystem::directory_iterator(log_directory)) {
        if (entry.is_directory()) {
          std::string sub_path = entry.path().string() + "/motor_states.csv";
          if (std::filesystem::exists(sub_path)) {
            motor_states_path = sub_path;
            break;
          }
        }
      }
    }

    if (motor_states_path.empty()) {
      std::cout << "❌ Could not find motor_states.csv in: " << log_directory
                << std::endl;
      return false;
    }

    std::cout << "📂 Loading motor states from: " << motor_states_path
              << std::endl;

    std::ifstream file(motor_states_path);
    if (!file.is_open()) {
      std::cout << "❌ Failed to open motor_states.csv file" << std::endl;
      return false;
    }

    std::string line;
    bool header_processed = false;
    std::string first_timestamp;
    int data_line_count = 0;

    while (std::getline(file, line)) {
      if (line.empty())
        continue;

      // Skip header line
      if (!header_processed) {
        header_processed = true;
        continue;
      }

      data_line_count++;

      // Skip first 100 data points to avoid initial unstable data
      if (data_line_count <= 100) {
        continue;
      }

      std::vector<double> row_data = parse_csv_line(line);
      if (row_data.size() <
          28) { // Expecting timestamp + 27 data points (9 motors * 3 values)
        std::cout << "⚠️ Skipping malformed row with " << row_data.size()
                  << " columns" << std::endl;
        continue;
      }

      // Extract timestamp (first column)
      std::string timestamp_str = extract_timestamp_from_line(line);
      if (first_timestamp.empty()) {
        first_timestamp = timestamp_str;
      }

      // Calculate relative time in seconds from first timestamp
      double relative_time =
          calculate_time_difference(first_timestamp, timestamp_str);
      time_points.push_back(relative_time);

      // Extract positions (every 3rd column starting from index 1:
      // position_motor_1, position_motor_2, etc.)
      std::vector<double> motor_positions;
      for (int motor = 0; motor < 9; motor++) {
        int pos_index = 1 + motor * 3; // position_motor_N is at column 1 + N*3
        if (pos_index < row_data.size()) {
          motor_positions.push_back(row_data[pos_index]);
        } else {
          motor_positions.push_back(0.0); // Default if not found
        }
      }
      positions.push_back(motor_positions);
    }

    std::cout
        << "📋 Loaded " << positions.size()
        << " motor state points from CSV (skipped first 100 unstable points)"
        << std::endl;

    // Estimate frequency from timestamps
    if (time_points.size() > 1) {
      frequency = 1.0 / (time_points[1] - time_points[0]);
    } else {
      frequency = 400.0; // Default logging frequency
    }

    std::cout << "✅ Loaded " << positions.size() << " motor state samples"
              << std::endl;
    std::cout << "📊 Estimated frequency: " << std::fixed
              << std::setprecision(1) << frequency << " Hz" << std::endl;
    std::cout << "⏱️  Duration: " << std::fixed << std::setprecision(2)
              << (time_points.empty() ? 0.0 : time_points.back()) << " seconds"
              << std::endl;

    return !positions.empty();
  }

private:
  static std::vector<double> parse_csv_line(const std::string &line) {
    std::vector<double> result;
    std::string cell;
    bool in_quotes = false;

    for (char c : line) {
      if (c == '"') {
        in_quotes = !in_quotes;
      } else if (c == ',' && !in_quotes) {
        if (!cell.empty()) {
          try {
            result.push_back(std::stod(cell));
          } catch (...) {
            result.push_back(0.0);
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
        result.push_back(std::stod(cell));
      } catch (...) {
        result.push_back(0.0);
      }
    }

    return result;
  }

  static std::string extract_timestamp_from_line(const std::string &line) {
    size_t comma_pos = line.find(',');
    if (comma_pos != std::string::npos) {
      return line.substr(0, comma_pos);
    }
    return line;
  }

  static double
  calculate_time_difference(const std::string &start_timestamp,
                            const std::string &current_timestamp) {
    // Parse ISO format timestamps like "2025-11-01T15:16:02.805"
    try {
      // Simple parsing - extract hours, minutes, seconds, milliseconds
      size_t time_pos = current_timestamp.find('T');
      if (time_pos == std::string::npos)
        return 0.0;

      std::string time_str = current_timestamp.substr(time_pos + 1);

      // Parse HH:MM:SS.mmm format
      double hours = 0.0, minutes = 0.0, seconds = 0.0;
      char sep;
      std::istringstream iss(time_str);
      iss >> hours >> sep >> minutes >> sep >> seconds;

      return hours * 3600.0 + minutes * 60.0 + seconds;
    } catch (...) {
      return 0.0;
    }
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

  bool load_from_file_or_directory(const std::string &input_path) {
    // Check if it's a directory (log directory) or file (JSON)
    if (std::filesystem::is_directory(input_path)) {
      return load_from_log_directory(input_path);
    } else if (std::filesystem::exists(input_path) &&
               input_path.substr(input_path.find_last_of(".") + 1) == "json") {
      return load_from_json(input_path);
    } else {
      std::cout << "❌ Invalid input path. Must be a JSON file or log "
                   "directory containing motor_states.csv"
                << std::endl;
      return false;
    }
  }

  bool load_from_json(const std::string &json_file) {
    std::cout << "📂 Loading trajectory from: " << json_file << std::endl;

    try {
      // Use our simple JSON parser
      if (!SimpleJSONParser::parse_trajectory(json_file, frequency, time_points,
                                              positions)) {
        std::cout << "❌ Failed to parse JSON file" << std::endl;
        return false;
      }

      std::cout << "📊 Trajectory frequency: " << frequency << " Hz"
                << std::endl;
      std::cout << "⏱️  Time points loaded: " << time_points.size() << std::endl;
      std::cout << "📍 Position waypoints loaded: " << positions.size()
                << std::endl;

      // Validate data consistency
      if (time_points.size() != positions.size()) {
        std::cout << "❌ ERROR: Time and position arrays have different sizes"
                  << std::endl;
        return false;
      }

      // Calculate trajectory properties
      total_points = positions.size();
      duration = time_points.back();
      std::cout << "⏱️  Trajectory duration: " << std::fixed
                << std::setprecision(2) << duration << " seconds" << std::endl;
      std::cout << "📍 Total trajectory points: " << total_points << std::endl;

      // Check position dimensions
      if (!positions.empty()) {
        size_t dof = positions[0].size();
        std::cout << "🦾 Degrees of freedom: " << dof << std::endl;

        // Validate all position vectors have same dimension
        for (size_t i = 0; i < positions.size(); i++) {
          if (positions[i].size() != dof) {
            std::cout << "❌ ERROR: Inconsistent DOF at point " << i
                      << std::endl;
            return false;
          }
        }

        // Pad to 9 motors if needed (add zeros for missing motors)
        if (dof < 9) {
          std::cout << "📝 Padding from " << dof
                    << " to 9 motors (adding zeros)" << std::endl;
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

  bool load_from_log_directory(const std::string &log_directory) {
    std::cout << "📂 Loading motor states from log directory: " << log_directory
              << std::endl;

    try {
      // Use our CSV parser to load motor states
      if (!MotorStateCSVParser::parse_motor_states(log_directory, frequency,
                                                   time_points, positions)) {
        std::cout << "❌ Failed to parse motor states from log directory"
                  << std::endl;
        return false;
      }

      std::cout << "📊 Replay frequency: " << frequency << " Hz" << std::endl;
      std::cout << "⏱️ Time points loaded: " << time_points.size() << std::endl;
      std::cout << "📍 Motor position samples loaded: " << positions.size()
                << std::endl;

      // Validate data consistency
      if (time_points.size() != positions.size()) {
        std::cout << "❌ ERROR: Time and position arrays have different sizes"
                  << std::endl;
        return false;
      }

      // Calculate trajectory properties
      total_points = positions.size();
      duration = time_points.back();
      std::cout << "⏱️  Replay duration: " << std::fixed << std::setprecision(2)
                << duration << " seconds" << std::endl;
      std::cout << "📍 Total motor state points: " << total_points << std::endl;

      // Check position dimensions (should be 9 for motors 1-9)
      if (!positions.empty()) {
        size_t dof = positions[0].size();
        std::cout << "🦾 Motors loaded: " << dof << std::endl;

        // Validate all position vectors have same dimension
        for (size_t i = 0; i < positions.size(); i++) {
          if (positions[i].size() != dof) {
            std::cout << "❌ ERROR: Inconsistent motor count at sample " << i
                      << std::endl;
            return false;
          }
        }

        // Ensure we have exactly 9 motors
        if (dof < 9) {
          std::cout << "📝 Padding from " << dof
                    << " to 9 motors (adding zeros)" << std::endl;
          for (auto &pos : positions) {
            pos.resize(9, 0.0);
          }
        } else if (dof > 9) {
          std::cout << "📝 Truncating from " << dof
                    << " to 9 motors (using first 9)" << std::endl;
          for (auto &pos : positions) {
            pos.resize(9, 0.0);
          }
        }
      }

      std::cout << "✅ Motor state trajectory loaded successfully" << std::endl;
      return true;

    } catch (const std::exception &e) {
      std::cout << "❌ Error loading motor states: " << e.what() << std::endl;
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
        std::cout << std::fixed << std::setprecision(3) << positions[0][i]
                  << " ";
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
  std::cout << "Usage: " << program_name
            << " [options] <trajectory_file.json|log_directory>" << std::endl;
  std::cout << "Options:" << std::endl;
  std::cout << "  -l             Enable logging" << std::endl;
  std::cout << "  -h             Show this help message" << std::endl;
  std::cout << "\nExamples:" << std::endl;
  std::cout
      << "  " << program_name
      << " trajectory.json                    # Execute trajectory from JSON"
      << std::endl;
  std::cout << "  " << program_name
            << " arm_monitor_20251101_231602/        # Replay motor states "
               "from log directory"
            << std::endl;
  std::cout << "  " << program_name
            << " -l trajectory.json                  # Execute with logging"
            << std::endl;
  std::cout << "  " << program_name
            << " -l arm_monitor_20251101_231602/    # Replay with logging"
            << std::endl;
  std::cout << "\nLog directory must contain motor_states.csv file"
            << std::endl;
}

int main(int argc, char *argv[]) {
  std::cout << "=== IC_CAN Trajectory Executor ===" << std::endl;
  std::cout << "Load and execute trajectories from JSON files or replay motor "
               "states from log directories"
            << std::endl;

  // Parse command line arguments
  bool enable_logging = false;
  std::string input_path; // Can be JSON file or log directory

  for (int i = 1; i < argc; i++) {
    std::string arg = argv[i];

    if (arg == "-h" || arg == "--help") {
      print_usage(argv[0]);
      return 0;
    } else if (arg == "-l" || arg == "--log") {
      enable_logging = true;
    } else if (input_path.empty()) {
      input_path = arg;
    } else {
      std::cout << "❌ Unknown argument: " << arg << std::endl;
      print_usage(argv[0]);
      return -1;
    }
  }

  if (input_path.empty()) {
    std::cout << "❌ ERROR: No trajectory file or log directory specified"
              << std::endl;
    print_usage(argv[0]);
    return -1;
  }

  signal(SIGINT, signal_handler);
  signal(SIGTERM, signal_handler);

  // Frequency monitoring variables
  auto freq_monitor_start = std::chrono::high_resolution_clock::now();
  int motor1_command_count = 0;
  auto last_freq_display = freq_monitor_start;

  try {
    // Load trajectory data (from JSON file or log directory)
    TrajectoryData trajectory;
    if (!trajectory.load_from_file_or_directory(input_path)) {
      return -1;
    }

    trajectory.print_info();

    // Create IC_CAN controller
    std::cout << "\n🔧 Initializing IC_CAN controller..." << std::endl;
    auto controller = std::make_unique<ic_can::IC_CAN>(
        "693D3DE86DF5940C8BC74A5B46A3CE2E", false);

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

    // Set to EXECUTION_MODE for trajectory execution
    std::cout << "\n⚙️ Setting EXECUTION_MODE for trajectory execution..."
              << std::endl;
    controller->set_control_mode(ic_can::IC_CAN::ControlMode::EXECUTION_MODE);

    // Enable compensation for trajectory execution
    /*controller->enable_gravity_compensation();*/
    /*controller->enable_friction_compensation();*/

    // Enable logging for trajectory execution (always enabled to capture replay
    // data)
    std::cout << "\n📝 Starting trajectory execution logging..." << std::endl;
    if (!controller->start_logging("./logs")) {
      std::cout << "❌ FAILED: Could not start logging" << std::endl;
    } else {
      std::cout << "✅ Execution logging enabled" << std::endl;
    }

    // Enable frequency monitoring
    controller->enable_frequency_monitoring();

    // Read initial positions
    controller->refresh_all();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    auto initial_positions = controller->get_joint_positions();

    std::cout << "\n📍 Current motor positions: ";
    for (int i = 0; i < 6; i++) {
      std::cout << std::fixed << std::setprecision(3) << initial_positions[i]
                << " ";
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
      controller->set_target_positions_interpolated(trajectory.positions[0],
                                                    0.5);
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

    // Performance profiling variables
    auto profiling_start = std::chrono::high_resolution_clock::now();
    int commands_in_last_second = 0;
    auto last_profiling_time = profiling_start;

    // Store timing stats for reporting (static to persist between loop
    // iterations)
    static double last_loop_time_ms = 0.0;
    static double last_sleep_time_ms = 0.0;

    while (g_running && current_point < trajectory.total_points) {
      auto loop_start = std::chrono::high_resolution_clock::now();

      // Check for large position changes (potential cause of jerky motion)
      static std::vector<double> last_positions(9, 0.0);
      double max_position_change = 0.0;
      int motor_with_max_change = 0;
      for (int i = 0; i < 9; i++) {
        double change = std::abs(trajectory.positions[current_point][i] -
                                 last_positions[i]);
        if (change > max_position_change) {
          max_position_change = change;
          motor_with_max_change = i;
        }
      }

      // Warn about large position changes
      if (max_position_change > 0.1) { // > 0.1 rad (~5.7 degrees)
        std::cout << "⚠️  Large position jump detected: Motor "
                  << (motor_with_max_change + 1) << " change=" << std::fixed
                  << std::setprecision(4) << max_position_change << " rad ("
                  << (max_position_change * 180.0 / M_PI) << "°)" << std::endl;
      }

      // Apply position directly without filtering for maximum fidelity
      const auto &target_positions = trajectory.positions[current_point];

      // Measure set_joint_positions timing
      auto cmd_start = std::chrono::high_resolution_clock::now();
      controller->set_joint_positions(target_positions, {}, {});
      auto cmd_end = std::chrono::high_resolution_clock::now();

      // Count motor 1 commands (each set_joint_positions sends to all motors
      // including motor 1)
      motor1_command_count++;
      commands_in_last_second++;

      current_point++;

      // Detailed profiling every 1 second
      auto current_time = std::chrono::high_resolution_clock::now();
      auto profiling_elapsed =
          std::chrono::duration<double>(current_time - last_profiling_time)
              .count();

      if (profiling_elapsed >= 1.0) {
        // Use trajectory execution start time for accurate frequency
        // calculation
        double execution_elapsed =
            std::chrono::duration<double>(current_time - start_time).count();
        double actual_freq = motor1_command_count / execution_elapsed;
        double instant_freq = commands_in_last_second / profiling_elapsed;

        auto cmd_duration =
            std::chrono::duration<double, std::milli>(cmd_end - cmd_start)
                .count();

        std::cout << "📊 ===== FREQUENCY PROFILING =====" << std::endl;
        std::cout << "   Motor 1 Total Freq: " << std::fixed
                  << std::setprecision(1) << actual_freq << " Hz" << std::endl;
        std::cout << "   Motor 1 Instant Freq: " << std::fixed
                  << std::setprecision(1) << instant_freq << " Hz" << std::endl;
        std::cout << "   Target Frequency: " << trajectory.frequency << " Hz"
                  << std::endl;
        std::cout << "   Efficiency: " << std::fixed << std::setprecision(1)
                  << (actual_freq / trajectory.frequency * 100.0) << "%"
                  << std::endl;
        std::cout << "   Command Time: " << std::fixed << std::setprecision(3)
                  << cmd_duration << " ms" << std::endl;
        std::cout << "   Loop Time: " << std::fixed << std::setprecision(3)
                  << last_loop_time_ms << " ms" << std::endl;
        std::cout << "   Sleep Time: " << std::fixed << std::setprecision(3)
                  << last_sleep_time_ms << " ms" << std::endl;
        std::cout << "   Target Period: " << std::fixed << std::setprecision(3)
                  << (1000.0 / trajectory.frequency) << " ms" << std::endl;
        std::cout << "   Max Position Change: " << std::fixed
                  << std::setprecision(4) << max_position_change << " rad"
                  << std::endl;
        std::cout << "   Commands this second: " << commands_in_last_second
                  << std::endl;
        std::cout << "   Total commands: " << motor1_command_count << std::endl;
        std::cout << "   Current point: " << current_point << "/"
                  << trajectory.total_points << std::endl;

        // Show current control mode and gains
        auto current_mode = controller->get_control_mode();
        std::cout << "   Control Mode: "
                  << (current_mode ==
                              ic_can::IC_CAN::ControlMode::EXECUTION_MODE
                          ? "EXECUTION"
                          : "TEACH")
                  << std::endl;

        // Alert if timing is problematic
        if (last_loop_time_ms > 5.0) {
          std::cout << "⚠️  WARNING: Loop time too high (>5ms)" << std::endl;
        }
        if (actual_freq < trajectory.frequency * 0.8) {
          std::cout << "⚠️  WARNING: Frequency below 80% of target" << std::endl;
        }

        // Reset counters
        commands_in_last_second = 0;
        last_profiling_time = current_time;
      }

      // Print progress every second
      auto elapsed = std::chrono::duration<double>(
                         std::chrono::high_resolution_clock::now() - start_time)
                         .count();
      static int last_progress_time = -1;
      int current_time_int = static_cast<int>(elapsed);
      if (current_time_int != last_progress_time) {
        double progress =
            (double)current_point / trajectory.total_points * 100.0;
        std::cout << "⏱️  Time: " << std::setw(3) << current_time_int << "s | "
                  << "Progress: " << std::fixed << std::setprecision(1)
                  << progress << "% | Point: " << current_point << "/"
                  << trajectory.total_points << std::endl;
        last_progress_time = current_time_int;
      }

      // Calculate sleep time to maintain trajectory frequency
      auto loop_end = std::chrono::high_resolution_clock::now();
      auto loop_duration = loop_end - loop_start;
      auto target_period =
          std::chrono::duration<double>(1.0 / trajectory.frequency);
      auto sleep_time = target_period - loop_duration;

      // Sleep timing profiling
      auto sleep_start = std::chrono::high_resolution_clock::now();
      if (sleep_time.count() > 0) {
        std::this_thread::sleep_for(sleep_time);
      }
      auto sleep_end = std::chrono::high_resolution_clock::now();
      auto actual_sleep =
          std::chrono::duration<double, std::milli>(sleep_end - sleep_start)
              .count();

      // Store timing stats for reporting (update last sample)
      last_loop_time_ms =
          std::chrono::duration<double, std::milli>(loop_duration).count();
      last_sleep_time_ms = actual_sleep;
    }

    auto total_time =
        std::chrono::duration<double>(
            std::chrono::high_resolution_clock::now() - start_time)
            .count();

    std::cout << "\n✅ Trajectory execution completed!" << std::endl;
    std::cout << "   Total time: " << std::fixed << std::setprecision(2)
              << total_time << " seconds" << std::endl;

    // Final frequency statistics and position change profile
    double final_elapsed =
        std::chrono::duration<double>(
            std::chrono::high_resolution_clock::now() - start_time)
            .count();
    double final_avg_freq = motor1_command_count / final_elapsed;

    std::cout << "\n📊 ===== FINAL EXECUTION STATISTICS =====" << std::endl;
    std::cout << "📈 Motor 1 Final Statistics:" << std::endl;
    std::cout << "   Commands sent: " << motor1_command_count << std::endl;
    std::cout << "   Average frequency: " << std::fixed << std::setprecision(2)
              << final_avg_freq << " Hz" << std::endl;
    std::cout << "   Target frequency: " << trajectory.frequency << " Hz"
              << std::endl;
    std::cout << "   Frequency accuracy: " << std::fixed << std::setprecision(1)
              << (final_avg_freq / trajectory.frequency * 100.0) << "%"
              << std::endl;
    std::cout << "   Points executed: " << current_point << "/"
              << trajectory.total_points << std::endl;

    // Display position change profile for all motors
    std::cout << "\n📝 ===== POSITION CHANGE PROFILE (9 Motors) ====="
              << std::endl;

    // Position change statistics for each motor
    std::vector<int> change_counts(9, 0);
    std::vector<double> sum_changes(9, 0.0);
    std::vector<double> max_changes(9, 0.0);
    std::vector<double> avg_changes(9, 0.0);
    std::vector<int> large_jump_counts(9, 0); // > 0.1 rad changes

    // Calculate position change statistics from the trajectory data
    std::vector<double> prev_positions = (trajectory.positions.size() > 0)
                                             ? trajectory.positions[0]
                                             : std::vector<double>(9, 0.0);

    for (size_t point = 1;
         point < trajectory.positions.size() && point < (size_t)current_point;
         ++point) {
      for (int motor = 0;
           motor < 9 && motor < (int)trajectory.positions[point].size();
           ++motor) {
        double change = std::abs(trajectory.positions[point][motor] -
                                 prev_positions[motor]);

        sum_changes[motor] += change;
        change_counts[motor]++;

        if (change > max_changes[motor]) {
          max_changes[motor] = change;
        }

        if (change > 0.1) { // > 5.73 degrees
          large_jump_counts[motor]++;
        }

        prev_positions[motor] = trajectory.positions[point][motor];
      }
    }

    // Display per-motor statistics
    for (int motor = 0; motor < 9; ++motor) {
      avg_changes[motor] = (change_counts[motor] > 0)
                               ? sum_changes[motor] / change_counts[motor]
                               : 0.0;

      std::cout << "🦾 Motor " << (motor + 1) << " (";

      // Motor type labels
      const char *motor_types[] = {"DM10010L", "DM6248", "DM6248",
                                   "DM4340",   "DM4340", "DM4310",
                                   "HT4438",   "HT4438", "SERVO"};

      std::cout << motor_types[motor] << "):" << std::endl;
      std::cout << "   Total changes: " << change_counts[motor] << std::endl;
      std::cout << "   Max change: " << std::fixed << std::setprecision(4)
                << max_changes[motor] << " rad ("
                << (max_changes[motor] * 180.0 / M_PI) << "°)" << std::endl;
      std::cout << "   Avg change: " << std::fixed << std::setprecision(4)
                << avg_changes[motor] << " rad ("
                << (avg_changes[motor] * 180.0 / M_PI) << "°)" << std::endl;
      std::cout << "   Large jumps (>5.7°): " << large_jump_counts[motor]
                << std::endl;

      if (large_jump_counts[motor] > 0) {
        std::cout << "   ⚠️  Motor " << (motor + 1) << " has jerky motion risk!"
                  << std::endl;
      }

      if (motor < 8) {
        std::cout << std::endl; // Extra space between motors 1-8
      }
    }

    // Overall summary
    int total_changes = 0;
    int total_large_jumps = 0;
    for (int motor = 0; motor < 9; ++motor) {
      total_changes += change_counts[motor];
      total_large_jumps += large_jump_counts[motor];
    }

    std::cout << "\n📋 ===== OVERALL SUMMARY =====" << std::endl;
    std::cout << "   Total position changes across all motors: "
              << total_changes << std::endl;
    std::cout << "   Total large jumps (>5.7°): " << total_large_jumps
              << std::endl;
    std::cout << "   Average changes per motor: " << (total_changes / 9.0)
              << std::endl;

    if (total_large_jumps > 0) {
      std::cout << "\n⚠️  WARNING: " << total_large_jumps
                << " large position jumps detected!" << std::endl;
      std::cout << "   This explains the 'ka-ka-ka' jerky motion." << std::endl;
      std::cout << "   Consider using smoother trajectory data or applying "
                   "stronger filtering."
                << std::endl;
    } else {
      std::cout << "\n✅ No large position jumps detected - trajectory should "
                   "be smooth."
                << std::endl;
    }

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
