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

#include <algorithm>
#include <chrono>
#include <cmath>
#include <csignal>
#include <filesystem>
#include <fstream>
#include <ic_can/core/ic_can.hpp>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

// Memory debugging (simple approach to avoid macro conflicts)
#include <cassert>
#include <cstring>

// Simple debug function
inline void traj_debug_print(const std::string& msg, int line) {
    std::cout << "🐛 DEBUG: " << msg << " (line " << line << ")" << std::endl;
}

#define TRAJ_DEBUG_PRINT(msg) traj_debug_print(msg, __LINE__)

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
static std::mutex g_trajectory_mutex; // Mutex for thread-safe operations

void signal_handler(int signal) {
  std::cout << "\n⚠️  Received signal " << signal << ", stopping trajectory..."
            << std::endl;
  g_running = false;
}

// Enhanced connection validation with retry mechanism
bool validate_controller_connection(ic_can::IC_CAN* controller, int max_retries = 3) {
  if (!controller) {
    std::cout << "❌ ERROR: Controller pointer is null" << std::endl;
    return false;
  }

  for (int attempt = 1; attempt <= max_retries; ++attempt) {
    std::cout << "🔍 Validating controller connection (attempt " << attempt
              << "/" << max_retries << ")..." << std::endl;

    try {
      // Test basic connectivity by refreshing motor states
      controller->refresh_all();
      std::this_thread::sleep_for(std::chrono::milliseconds(100));

      // Check if we can read positions
      auto test_positions = controller->get_joint_positions();

      if (test_positions.size() >= 9) {
        std::cout << "✅ Controller connection validated successfully" << std::endl;
        return true;
      } else {
        std::cout << "⚠️  WARNING: Got insufficient position data ("
                  << test_positions.size() << " values, expected 9)" << std::endl;
      }

    } catch (const std::exception& e) {
      std::cout << "❌ Connection test failed: " << e.what() << std::endl;
    }

    if (attempt < max_retries) {
      std::cout << "🔄 Retrying in 1 second..." << std::endl;
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }
  }

  std::cout << "❌ ERROR: Failed to validate controller connection after "
            << max_retries << " attempts" << std::endl;
  return false;
}

// Safe vector access helper with bounds checking
double safe_vector_access(const std::vector<double>& vec, size_t index, double default_value = 0.0) {
  std::lock_guard<std::mutex> lock(g_trajectory_mutex);
  return (index < vec.size()) ? vec[index] : default_value;
}

// Safe vector resize with validation
void safe_vector_resize(std::vector<double>& vec, size_t target_size, double fill_value = 0.0) {
  std::lock_guard<std::mutex> lock(g_trajectory_mutex);
  if (vec.size() != target_size) {
    vec.resize(target_size, fill_value);
  }
}

// Custom exception handler to catch memory issues
void my_terminate() {
  std::cout << "🔥 CRITICAL: Program terminating - likely memory corruption detected!" << std::endl;
  std::cout << "🔥 This suggests a double free or invalid memory access" << std::endl;
  std::abort();
}

void setup_debug_handlers() {
    std::set_terminate(my_terminate);
    std::cout << "🐛 DEBUG: Debug handlers installed" << std::endl;
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
  std::cout << "  -i             Enable real-time interpolation (smooth motion)"
            << std::endl;
  std::cout << "  -v <max_vel>   Maximum velocity for interpolation (rad/s, "
               "default: 1.0)"
            << std::endl;
  std::cout << "  -s <smooth>    Enable trajectory preprocessing smoothing "
               "(0-1, default: 0)"
            << std::endl;
  std::cout << "  --servo-100x   Enable 1:100 servo control ratio (smooth "
               "gripper motion)"
            << std::endl;
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
            << " -i trajectory.json                  # Execute with "
               "interpolation (smooth)"
            << std::endl;
  std::cout << "  " << program_name
            << " -i -v 0.5 trajectory.json           # Execute with limited "
               "velocity (0.5 rad/s)"
            << std::endl;
  std::cout << "  " << program_name
            << " --servo-100x trajectory.json       # Execute with smooth "
               "gripper control"
            << std::endl;
  std::cout << "  " << program_name
            << " -l arm_monitor_20251101_231602/    # Replay with logging"
            << std::endl;
  std::cout << "\nLog directory must contain motor_states.csv file"
            << std::endl;
}

// Real-time interpolation function for smooth motion
void interpolate_positions(const std::vector<double> &current_positions,
                          const std::vector<double> &target_positions,
                          std::vector<double> &result_positions, double dt,
                          double max_velocity) {

  // Ensure result vector has correct size
  if (result_positions.size() != current_positions.size()) {
    result_positions.resize(current_positions.size());
  }

  double max_step = max_velocity * dt;

  for (size_t i = 0; i < current_positions.size(); ++i) {
    double error = target_positions[i] - current_positions[i];
    double step = std::clamp(error, -max_step, max_step);
    result_positions[i] = current_positions[i] + step;
  }
}

// Apply trajectory preprocessing smoothing
void smooth_trajectory(TrajectoryData &trajectory, double smooth_factor) {
  if (smooth_factor <= 0 || trajectory.positions.empty()) {
    return;
  }

  size_t num_joints = trajectory.positions[0].size();
  size_t num_points = trajectory.positions.size();

  for (size_t joint = 0; joint < num_joints; ++joint) {
    // Simple moving average smoothing
    std::vector<double> smoothed_values(num_points);
    int window_size =
        static_cast<int>(smooth_factor * 10) + 1; // Window size 1-11

    for (size_t i = 0; i < num_points; ++i) {
      double sum = 0.0;
      int count = 0;

      for (int j = -window_size / 2; j <= window_size / 2; ++j) {
        int idx = static_cast<int>(i) + j;
        if (idx >= 0 && idx < static_cast<int>(num_points)) {
          sum += trajectory.positions[idx][joint];
          count++;
        }
      }

      smoothed_values[i] = sum / count;
    }

    // Copy smoothed values back
    for (size_t i = 0; i < num_points; ++i) {
      trajectory.positions[i][joint] = smoothed_values[i];
    }
  }
}

int main(int argc, char *argv[]) {
  std::cout << "=== IC_CAN Trajectory Executor ===" << std::endl;
  std::cout << "Load and execute trajectories from JSON files or replay motor "
               "states from log directories"
            << std::endl;

  // Setup debug handlers for memory debugging
  setup_debug_handlers();

  TRAJ_DEBUG_PRINT("Main function started");

  // Parse command line arguments
  bool enable_logging = false;
  bool enable_interpolation = false;
  bool enable_servo_100x = false;
  double max_velocity = 1.0;  // rad/s
  double smooth_factor = 0.0; // 0 = no smoothing, 1 = maximum smoothing
  std::string input_path;     // Can be JSON file or log directory

  for (int i = 1; i < argc; i++) {
    std::string arg = argv[i];

    if (arg == "-h" || arg == "--help") {
      print_usage(argv[0]);
      return 0;
    } else if (arg == "-l" || arg == "--log") {
      enable_logging = true;
    } else if (arg == "-i" || arg == "--interpolate") {
      enable_interpolation = true;
    } else if ((arg == "-v" || arg == "--velocity") && i + 1 < argc) {
      try {
        max_velocity = std::stod(argv[++i]);
        if (max_velocity <= 0 || max_velocity > 10.0) {
          std::cout << "❌ Invalid max_velocity: " << max_velocity
                    << " (must be 0-10 rad/s)" << std::endl;
          return -1;
        }
      } catch (const std::exception &e) {
        std::cout << "❌ Invalid velocity value: " << argv[i] << std::endl;
        return -1;
      }
    } else if ((arg == "-s" || arg == "--smooth") && i + 1 < argc) {
      try {
        smooth_factor = std::stod(argv[++i]);
        if (smooth_factor < 0 || smooth_factor > 1) {
          std::cout << "❌ Invalid smooth factor: " << smooth_factor
                    << " (must be 0-1)" << std::endl;
          return -1;
        }
      } catch (const std::exception &e) {
        std::cout << "❌ Invalid smooth factor value: " << argv[i] << std::endl;
        return -1;
      }
    } else if (arg == "--servo-100x") {
      enable_servo_100x = true;
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
    TRAJ_DEBUG_PRINT("About to create trajectory object");
    TrajectoryData trajectory;
    TRAJ_DEBUG_PRINT("Trajectory object created, about to load data");
    if (!trajectory.load_from_file_or_directory(input_path)) {
      TRAJ_DEBUG_PRINT("Failed to load trajectory data");
      return -1;
    }
    TRAJ_DEBUG_PRINT("Trajectory data loaded successfully");

    trajectory.print_info();

    // Apply trajectory preprocessing smoothing if requested
    if (smooth_factor > 0) {
      std::cout << "🔧 Applying trajectory smoothing (factor: " << smooth_factor
                << ")..." << std::endl;
      smooth_trajectory(trajectory, smooth_factor);
      std::cout << "✅ Trajectory smoothing completed" << std::endl;
    }

    // Create IC_CAN controller
    TRAJ_DEBUG_PRINT("About to create IC_CAN controller");
    std::cout << "\n🔧 Initializing IC_CAN controller..." << std::endl;
    auto controller = std::make_unique<ic_can::IC_CAN>(
        "693D3DE86DF5940C8BC74A5B46A3CE2E", false);
    TRAJ_DEBUG_PRINT("IC_CAN controller created");

    if (!controller->initialize()) {
      std::cout << "❌ FAILED: System initialization failed" << std::endl;
      TRAJ_DEBUG_PRINT("System initialization failed - exiting");
      return -1;
    }
    std::cout << "✅ System initialized" << std::endl;
    TRAJ_DEBUG_PRINT("System initialized successfully");

    // Enable motors
    if (!controller->enable_all()) {
      std::cout << "⚠️  WARNING: Some motors failed to enable" << std::endl;
    }
    std::this_thread::sleep_for(std::chrono::microseconds(500000)); // 0.5 seconds instead of 1 second

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
    TRAJ_DEBUG_PRINT("About to refresh_all and get positions");
    controller->refresh_all();
    std::this_thread::sleep_for(std::chrono::microseconds(50000)); // 50ms instead of 200ms
    auto initial_positions = controller->get_joint_positions();
    TRAJ_DEBUG_PRINT("Got initial positions, size: " + std::to_string(initial_positions.size()));

    std::cout << "\n📍 Current motor positions: ";
    for (int i = 0; i < 9; i++) {
      double current_pos = (i < static_cast<int>(initial_positions.size())) ? initial_positions[i] : 0.0;
      std::cout << std::fixed << std::setprecision(3) << current_pos << " ";
    }
    std::cout << std::endl;

    std::cout << "📍 First trajectory position: ";
    size_t trajectory_dof = trajectory.positions[0].size();
    for (int i = 0; i < 9; i++) {
      double target_pos = (i < static_cast<int>(trajectory_dof)) ? trajectory.positions[0][i] : 0.0;
      std::cout << std::fixed << std::setprecision(3) << target_pos << " ";
    }
    std::cout << std::endl;

    // Check if we need to move to starting position (check all 9 motors)
    TRAJ_DEBUG_PRINT("Starting initial position check");
    bool need_initial_move = false;
    double max_distance = 0.0;
    std::vector<double> position_errors(9, 0.0);
    TRAJ_DEBUG_PRINT("Created position_errors vector, size: " + std::to_string(position_errors.size()));

    // Ensure we have enough elements in initial_positions
    if (initial_positions.size() < 9) {
        initial_positions.resize(9, 0.0);
        TRAJ_DEBUG_PRINT("Resized initial_positions to 9 elements");
    }

    TRAJ_DEBUG_PRINT("About to compare positions with trajectory[0] size: " + std::to_string(trajectory.positions[0].size()));

    // CRITICAL FIX: Add bounds checking to prevent buffer overflow
    TRAJ_DEBUG_PRINT("Trajectory DOF: " + std::to_string(trajectory_dof));

    for (int i = 0; i < 9; i++) {
      // SAFE ACCESS: Check bounds before accessing trajectory positions
      double target_pos = (i < static_cast<int>(trajectory_dof)) ? trajectory.positions[0][i] : 0.0;
      double current_pos = (i < static_cast<int>(initial_positions.size())) ? initial_positions[i] : 0.0;
      double error = std::abs(current_pos - target_pos);
      position_errors[i] = error;

      if (error > 0.01) { // 0.01 rad threshold
        need_initial_move = true;
      }
      if (error > max_distance) {
        max_distance = error;
      }
    }
    TRAJ_DEBUG_PRINT("Position comparison completed, need_initial_move: " + std::string(need_initial_move ? "true" : "false"));

    if (need_initial_move) {
      std::cout << "\n🎯 Moving to starting position..." << std::endl;
      std::cout << "   Max distance: " << std::fixed << std::setprecision(3)
                << max_distance << " rad (" << (max_distance * 180.0 / M_PI) << "°)" << std::endl;

      // Calculate dynamic initial move duration based on max distance and velocity
      double initial_move_velocity = std::min(max_velocity, 0.5); // Cap at 0.5 rad/s for safety
      double estimated_duration = max_distance / initial_move_velocity;
      double initial_move_duration = std::max(estimated_duration, 1.0); // Minimum 1 second

      std::cout << "   Using velocity: " << initial_move_velocity << " rad/s" << std::endl;
      std::cout << "   Estimated duration: " << std::fixed << std::setprecision(2)
                << initial_move_duration << " seconds" << std::endl;

      // Validate controller connection before starting control loop
      if (!validate_controller_connection(controller.get())) {
        std::cout << "❌ CRITICAL: Cannot proceed with initial movement - controller not connected" << std::endl;
        controller->disable_all();
        return -1;
      }

      // Use the same interpolation method as real-time execution
      double initial_frequency = 200.0; // 200Hz for initial move
      int initial_move_steps = static_cast<int>(initial_move_duration * initial_frequency);
      double dt = 1.0 / initial_frequency;

      TRAJ_DEBUG_PRINT("Creating interpolation vectors");

      try {
        // Safe vector creation with proper initialization
        std::vector<double> current_positions;
        std::vector<double> target_positions;
        std::vector<double> interpolated_positions(9, 0.0);

        // Initialize current_positions with safe copying
        current_positions.reserve(9);
        for (int i = 0; i < 9; ++i) {
          current_positions.push_back(safe_vector_access(initial_positions, i, 0.0));
        }

        // Initialize target_positions with safe copying from trajectory
        if (!trajectory.positions.empty() && trajectory.positions[0].size() >= 9) {
          target_positions.reserve(9);
          for (int i = 0; i < 9; ++i) {
            target_positions.push_back(trajectory.positions[0][i]);
          }
        } else {
          std::cout << "❌ ERROR: Invalid trajectory data for initial movement" << std::endl;
          controller->disable_all();
          return -1;
        }

        // Ensure all vectors have the correct size
        safe_vector_resize(current_positions, 9, 0.0);
        safe_vector_resize(target_positions, 9, 0.0);
        safe_vector_resize(interpolated_positions, 9, 0.0);

        TRAJ_DEBUG_PRINT("Created vectors - current size: " + std::to_string(current_positions.size()) +
                   ", target size: " + std::to_string(target_positions.size()) +
                   ", interpolated size: " + std::to_string(interpolated_positions.size()));

        // Start control loop with error handling
        std::cout << "🔧 Starting control loop for initial movement..." << std::endl;
        if (!controller->start_control_loop(initial_frequency)) {
          std::cout << "❌ CRITICAL: Failed to start control loop for initial movement" << std::endl;
          controller->disable_all();
          return -1;
        }

        std::cout << "✅ Control loop started successfully" << std::endl;

        for (int step = 0; step < initial_move_steps && g_running; ++step) {
          // Interpolate positions using the same function as real-time execution
          interpolate_positions(current_positions, target_positions, interpolated_positions,
                              dt, initial_move_velocity);

          // Send interpolated positions with error handling
          try {
            controller->set_joint_positions(interpolated_positions, {}, {});
          } catch (const std::exception& e) {
            std::cout << "❌ ERROR: Failed to send joint positions: " << e.what() << std::endl;
            controller->stop_control_loop();
            controller->disable_all();
            return -1;
          }

          // Update current positions for next iteration (safe copy)
          for (int i = 0; i < 9; ++i) {
            current_positions[i] = interpolated_positions[i];
          }

          // Sleep to maintain frequency
          std::this_thread::sleep_for(std::chrono::microseconds(static_cast<int64_t>(dt * 1000000)));
        }

        // Stop control loop safely
        try {
          controller->stop_control_loop();
          std::cout << "✅ Moved to starting position" << std::endl;
        } catch (const std::exception& e) {
          std::cout << "⚠️  WARNING: Error stopping control loop: " << e.what() << std::endl;
        }

      } catch (const std::exception& e) {
        std::cout << "❌ CRITICAL ERROR during initial movement: " << e.what() << std::endl;
        try {
          controller->stop_control_loop();
        } catch (...) {
          // Ignore cleanup errors
        }
        controller->disable_all();
        return -1;
      } catch (...) {
        std::cout << "❌ UNKNOWN CRITICAL ERROR during initial movement" << std::endl;
        try {
          controller->stop_control_loop();
        } catch (...) {
          // Ignore cleanup errors
        }
        controller->disable_all();
        return -1;
      }
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

    // Final connection validation before starting main trajectory execution
    std::cout << "🔍 Final connection validation before trajectory execution..." << std::endl;
    if (!validate_controller_connection(controller.get(), 2)) {
      std::cout << "❌ CRITICAL: Controller connection failed before trajectory execution" << std::endl;
      controller->disable_all();
      return -1;
    }
    std::cout << "✅ Final validation passed - starting trajectory execution" << std::endl;

    // Execute trajectory
    auto start_time = std::chrono::high_resolution_clock::now();
    size_t current_point = 0;

    // Print execution settings
    std::cout << "\n🚀 Starting trajectory execution..." << std::endl;
    std::cout << "   Frequency: " << trajectory.frequency << " Hz" << std::endl;
    std::cout << "   Duration: "
              << trajectory.total_points / trajectory.frequency << " seconds"
              << std::endl;
    std::cout << "   Total points: " << trajectory.total_points << std::endl;
    std::cout << "   Interpolation: "
              << (enable_interpolation ? "ENABLED" : "DISABLED") << std::endl;
    if (enable_interpolation) {
      std::cout << "   Max velocity: " << max_velocity << " rad/s" << std::endl;
      double dt = 1.0 / trajectory.frequency;
      double max_step = max_velocity * dt;
      std::cout << "   Max step: " << max_step << " rad ("
                << (max_step * 180.0 / M_PI) << "°)" << std::endl;
    }
    std::cout << "   Pre-smoothing: "
              << (smooth_factor > 0 ? "ENABLED" : "DISABLED");
    if (smooth_factor > 0) {
      std::cout << " (factor: " << smooth_factor << ")";
    }
    std::cout << std::endl;
    std::cout << "   Servo 100x Control: "
              << (enable_servo_100x ? "ENABLED" : "DISABLED") << std::endl;

    // Performance profiling variables
    auto profiling_start = std::chrono::high_resolution_clock::now();
    int commands_in_last_second = 0;
    auto last_profiling_time = profiling_start;

    // Servo 100x control variables
    int servo_control_counter = 0;
    const int servo_ratio =
        100; // 100 position updates for every 1 servo command
    std::vector<double> servo_target_position(
        9, 0.0); // Store target for servo interpolation
    std::vector<double> servo_current_position(
        9, 0.0); // Current interpolated position
    auto last_servo_command_time = start_time;
    bool servo_initialized = false;

    // Store timing stats for reporting (static to persist between loop
    // iterations)
    static double last_loop_time_ms = 0.0;
    static double last_sleep_time_ms = 0.0;

    while (g_running && current_point < trajectory.total_points) {
      auto loop_start = std::chrono::high_resolution_clock::now();

      try {
        // Thread-safe position jump detection (performance optimized)
        static std::vector<double> last_positions(9, 0.0);
        static bool first_iteration = true;
        static auto last_position_read = std::chrono::steady_clock::now();

        // Thread-safe cache motor positions to reduce USB reads (only read every 10 iterations)
        static std::vector<double> cached_motor_positions(9, 0.0);
        static bool positions_cached = false;
        const int position_cache_interval = 10;

        // Use mutex to protect static variables
        {
          std::lock_guard<std::mutex> lock(g_trajectory_mutex);

          if (current_point % position_cache_interval == 0 || !positions_cached) {
            auto temp_positions = controller->get_joint_positions();
            // Safe copy with proper size checking
            if (temp_positions.size() >= 9) {
              std::copy(temp_positions.begin(), temp_positions.begin() + 9,
                       cached_motor_positions.begin());
            } else {
              // Fallback if returned vector is too small
              for (size_t i = 0; i < 9; ++i) {
                cached_motor_positions[i] = (i < temp_positions.size()) ? temp_positions[i] : 0.0;
              }
            }
            positions_cached = true;
            last_position_read = std::chrono::steady_clock::now();
          }

          double max_position_change = 0.0;

          // Quick trajectory change detection only (no USB calls)
          for (int i = 0; i < 9; i++) {
            double trajectory_change = std::abs(
                safe_vector_access(trajectory.positions[current_point], i, 0.0) -
                safe_vector_access(last_positions, i, 0.0));
            if (trajectory_change > max_position_change) {
              max_position_change = trajectory_change;
            }
          }

          // Initialize last_positions on first iteration
          if (first_iteration) {
            if (current_point < trajectory.positions.size() && trajectory.positions[current_point].size() >= 9) {
              std::copy(trajectory.positions[current_point].begin(),
                       trajectory.positions[current_point].begin() + 9,
                       last_positions.begin());
            }
            first_iteration = false;
          }
        } // Mutex lock released here

        // Apply position with real-time interpolation if enabled
        std::vector<double> target_positions_safe;
        if (current_point < trajectory.positions.size() && trajectory.positions[current_point].size() >= 9) {
          target_positions_safe = trajectory.positions[current_point];
        } else {
          // Safe fallback
          target_positions_safe.resize(9, 0.0);
          if (current_point < trajectory.positions.size()) {
            for (size_t i = 0; i < std::min(trajectory.positions[current_point].size(), size_t(9)); ++i) {
              target_positions_safe[i] = trajectory.positions[current_point][i];
            }
          }
        }

        std::vector<double> actual_positions;

        if (enable_interpolation) {
          // Calculate time step
          double dt = 1.0 / trajectory.frequency;

          // Interpolate between cached current and target positions (no USB call)
          actual_positions = target_positions_safe; // Start with target positions

          // Use thread-safe access to cached positions
          std::vector<double> cached_copy(9, 0.0);
          {
            std::lock_guard<std::mutex> lock(g_trajectory_mutex);
            std::copy(cached_motor_positions.begin(), cached_motor_positions.end(),
                     cached_copy.begin());
          }

          interpolate_positions(cached_copy, target_positions_safe, actual_positions, dt, max_velocity);
        } else {
          // Use raw target positions (original behavior)
          actual_positions = target_positions_safe;
        }

        // Measure set_joint_positions timing (declare outside blocks for scope)
        auto cmd_start = std::chrono::high_resolution_clock::now();

        // Handle servo 100x control if enabled
        if (enable_servo_100x) {
          // Initialize servo positions on first iteration
          if (!servo_initialized) {
            servo_target_position = actual_positions;
            servo_current_position = actual_positions;
            servo_initialized = true;
          }

          // Update target position for servo interpolation
          servo_target_position = actual_positions;

          // Interpolate servo position (servo is motor 9, index 8)
          double servo_interpolation_factor = 1.0 / servo_ratio;
          double servo_step =
              (servo_target_position[8] - servo_current_position[8]) *
              servo_interpolation_factor;
          servo_current_position[8] += servo_step;

          // Create position vector: normal positions for motors 1-8, interpolated
          // for motor 9
          std::vector<double> servo_control_positions = actual_positions;
          servo_control_positions[8] =
              servo_current_position[8]; // Use interpolated servo position

          // Send interpolated positions to motors with error handling
          try {
            controller->set_joint_positions(servo_control_positions, {}, {});
          } catch (const std::exception& e) {
            std::cout << "❌ ERROR: Failed to send servo positions: " << e.what() << std::endl;
            // Don't abort entire trajectory for servo errors, just log them
          }

          // Increment servo counter
          servo_control_counter++;

          // Send actual servo command every 100 iterations
          if (servo_control_counter >= servo_ratio) {
            // Send direct servo command to ensure final position is reached
            try {
              controller->set_joint_positions(servo_target_position, {}, {});
            } catch (const std::exception& e) {
              std::cout << "❌ ERROR: Failed to send final servo command: " << e.what() << std::endl;
            }

            // Reset counter and update current position
            servo_control_counter = 0;
            servo_current_position[8] = servo_target_position[8];
            last_servo_command_time = std::chrono::high_resolution_clock::now();
          }

          // Count motor 1 commands (each set_joint_positions sends to all motors
          // including motor 1)
          motor1_command_count++;
          commands_in_last_second++;

        } else {
          // Standard control (original behavior)
          try {
            controller->set_joint_positions(actual_positions, {}, {});
          } catch (const std::exception& e) {
            std::cout << "❌ ERROR: Failed to send joint positions: " << e.what() << std::endl;
            // Don't abort entire trajectory for individual command errors
          }

          // Count motor 1 commands (each set_joint_positions sends to all motors
          // including motor 1)
          motor1_command_count++;
          commands_in_last_second++;
        }

        auto cmd_end = std::chrono::high_resolution_clock::now();

        current_point++;

        // Update last_positions for next iteration's delta calculations (thread-safe)
        {
          std::lock_guard<std::mutex> lock(g_trajectory_mutex);
          if (current_point < trajectory.total_points && current_point > 0) {
            size_t prev_point = current_point - 1;
            if (prev_point < trajectory.positions.size() && trajectory.positions[prev_point].size() >= 9) {
              std::copy(trajectory.positions[prev_point].begin(),
                       trajectory.positions[prev_point].begin() + 9,
                       last_positions.begin());
            }
          }
        } // Mutex lock released

      } catch (const std::exception& e) {
        std::cout << "❌ ERROR in trajectory execution loop: " << e.what() << std::endl;
        std::cout << "   Attempting to continue execution..." << std::endl;
        current_point++; // Skip this point and continue
      } catch (...) {
        std::cout << "❌ UNKNOWN ERROR in trajectory execution loop" << std::endl;
        std::cout << "   Attempting to continue execution..." << std::endl;
        current_point++; // Skip this point and continue
      }

      // Lightweight profiling every 2 seconds (reduced from 1 second)
      auto current_time = std::chrono::high_resolution_clock::now();
      auto profiling_elapsed =
          std::chrono::duration<double>(current_time - last_profiling_time)
              .count();

      if (profiling_elapsed >= 2.0) {
        // Simplified frequency calculation
        double execution_elapsed =
            std::chrono::duration<double>(current_time - start_time).count();
        double actual_freq = motor1_command_count / execution_elapsed;
        double instant_freq = commands_in_last_second / profiling_elapsed;

        // Minimal console output for performance
        std::cout << "📊 Freq: " << std::fixed << std::setprecision(1)
                  << actual_freq << "/" << trajectory.frequency << " Hz | "
                  << "Eff: " << std::setprecision(0)
                  << (actual_freq / trajectory.frequency * 100.0) << "% | "
                  << "Loop: " << std::setprecision(2) << last_loop_time_ms << "ms | "
                  << "Pt: " << current_point << "/" << trajectory.total_points << std::endl;

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

      // Calculate sleep time to maintain trajectory frequency (microsecond precision)
      auto loop_end = std::chrono::high_resolution_clock::now();
      auto loop_duration = loop_end - loop_start;
      double target_period_us = 1000000.0 / trajectory.frequency;
      double loop_duration_us = std::chrono::duration<double, std::micro>(loop_duration).count();
      double sleep_time_us = target_period_us - loop_duration_us;

      // Microsecond-precision sleep with minimal overhead
      auto sleep_start = std::chrono::high_resolution_clock::now();
      if (sleep_time_us > 100.0) { // Only sleep if > 100μs
        // Use busy-wait for small intervals (< 1ms) for better precision
        if (sleep_time_us < 1000.0) {
          auto target_time = sleep_start + std::chrono::microseconds(static_cast<int64_t>(sleep_time_us));
          while (std::chrono::high_resolution_clock::now() < target_time) {
            // CPU pause for better power efficiency
            __builtin_ia32_pause();
          }
        } else {
          // Use standard sleep for larger intervals
          std::this_thread::sleep_for(std::chrono::microseconds(static_cast<int64_t>(sleep_time_us - 100.0)));
          // Fine-tune with busy-wait for remaining time
          auto target_time = sleep_start + std::chrono::microseconds(static_cast<int64_t>(sleep_time_us));
          while (std::chrono::high_resolution_clock::now() < target_time) {
            __builtin_ia32_pause();
          }
        }
      }
      auto sleep_end = std::chrono::high_resolution_clock::now();
      auto actual_sleep_us =
          std::chrono::duration<double, std::micro>(sleep_end - sleep_start).count();

      // Store timing stats for reporting (convert to ms for display)
      last_loop_time_ms = loop_duration_us / 1000.0;
      last_sleep_time_ms = actual_sleep_us / 1000.0;
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
    TRAJ_DEBUG_PRINT("Caught std::exception: " + std::string(e.what()));
    return -1;
  } catch (...) {
    std::cout << "❌ UNKNOWN EXCEPTION CAUGHT!" << std::endl;
    TRAJ_DEBUG_PRINT("Caught unknown exception - likely memory corruption");
    return -1;
  }
}
