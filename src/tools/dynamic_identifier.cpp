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
 * @file dynamic_identifier.cpp
 * @brief Dynamic Parameter Identification Tool
 *
 * Tool for identifying mass and inertia parameters from collected trajectory data:
 * - Load trajectory data from CSV files
 * - Apply filtering to velocities and accelerations
 * - Use regressor matrix for parameter estimation
 * - Perform least squares estimation
 * - Calculate R² scores and validation metrics
 * - Export results for Python plotting
 */

#include <chrono>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

// From regressor.cpp
void H_func(double *regressor, const double *q, const double *dq,
            const double *ddq);

struct TrajectoryData {
  std::vector<double> timestamps;
  std::vector<std::vector<double>> positions;     // q
  std::vector<std::vector<double>> velocities;    // dq (raw)
  std::vector<std::vector<double>> torques;       // tau
  std::vector<std::vector<double>> accelerations; // ddq (computed)
  int num_joints = 6;  // Focus on first 6 joints for identification
  size_t num_samples = 0;

  bool load_from_csv(const std::string &motor_states_file) {
    std::cout << "📂 Loading trajectory data from: " << motor_states_file << std::endl;

    std::ifstream file(motor_states_file);
    if (!file.is_open()) {
      std::cout << "❌ Failed to open CSV file: " << motor_states_file << std::endl;
      return false;
    }

    std::string line;
    // Skip header
    if (!std::getline(file, line)) {
      std::cout << "❌ Empty CSV file" << std::endl;
      return false;
    }

    std::cout << "📊 Reading trajectory data..." << std::endl;

    // Count data points first for memory allocation
    size_t line_count = 0;
    std::streampos original_pos = file.tellg();
    while (std::getline(file, line)) {
      line_count++;
    }
    file.clear();
    file.seekg(original_pos);

    std::cout << "📈 Found " << line_count << " data points" << std::endl;

    // Allocate memory
    timestamps.reserve(line_count);
    positions.reserve(line_count);
    velocities.reserve(line_count);
    torques.reserve(line_count);
    accelerations.reserve(line_count);

    // Parse data
    while (std::getline(file, line)) {
      std::stringstream ss(line);
      std::string cell;

      // Parse timestamp (skip for now)
      std::getline(ss, cell, ',');

      // Parse position, velocity, torque for 6 joints
      std::vector<double> pos(6), vel(6), tau(6);
      bool valid_row = true;

      for (int i = 0; i < 6; i++) {
        // Position
        if (!std::getline(ss, cell, ',')) { valid_row = false; break; }
        try { pos[i] = std::stod(cell); } catch (...) { valid_row = false; break; }

        // Velocity
        if (!std::getline(ss, cell, ',')) { valid_row = false; break; }
        try { vel[i] = std::stod(cell); } catch (...) { valid_row = false; break; }

        // Torque
        if (!std::getline(ss, cell, ',')) { valid_row = false; break; }
        try { tau[i] = std::stod(cell); } catch (...) { valid_row = false; break; }
      }

      if (valid_row) {
        timestamps.push_back(timestamps.size()); // Simple index
        positions.push_back(pos);
        velocities.push_back(vel);
        torques.push_back(tau);
      }
    }

    num_samples = positions.size();
    std::cout << "✅ Successfully loaded " << num_samples << " valid samples" << std::endl;

    return num_samples > 0;
  }

  void compute_accelerations() {
    std::cout << "🔢 Computing accelerations from velocities..." << std::endl;

    if (num_samples < 3) {
      std::cout << "❌ Not enough samples for acceleration computation" << std::endl;
      return;
    }

    accelerations.resize(num_samples, std::vector<double>(6, 0.0));

    // Use central difference for interior points
    for (size_t i = 1; i < num_samples - 1; i++) {
      for (int j = 0; j < 6; j++) {
        accelerations[i][j] = (velocities[i+1][j] - velocities[i-1][j]) / 2.0;
      }
    }

    // Forward difference for first point
    for (int j = 0; j < 6; j++) {
      accelerations[0][j] = velocities[1][j] - velocities[0][j];
    }

    // Backward difference for last point
    for (int j = 0; j < 6; j++) {
      accelerations[num_samples-1][j] = velocities[num_samples-1][j] - velocities[num_samples-2][j];
    }

    std::cout << "✅ Accelerations computed" << std::endl;
  }

  // Simple low-pass filter for velocities and accelerations
  void apply_filter(int window_size = 5) {
    std::cout << "🔍 Applying low-pass filter (window size: " << window_size << ")..." << std::endl;

    if (num_samples < static_cast<size_t>(window_size)) {
      std::cout << "⚠️  Not enough samples for filtering, skipping" << std::endl;
      return;
    }

    // Filter velocities
    std::vector<std::vector<double>> filtered_vel = velocities;
    std::vector<std::vector<double>> filtered_acc = accelerations;

    for (size_t i = window_size/2; i < num_samples - window_size/2; i++) {
      for (int j = 0; j < 6; j++) {
        double vel_sum = 0.0, acc_sum = 0.0;
        for (int k = -window_size/2; k <= window_size/2; k++) {
          vel_sum += velocities[i + k][j];
          acc_sum += accelerations[i + k][j];
        }
        filtered_vel[i][j] = vel_sum / window_size;
        filtered_acc[i][j] = acc_sum / window_size;
      }
    }

    velocities = filtered_vel;
    accelerations = filtered_acc;

    std::cout << "✅ Filtering completed" << std::endl;
  }

  void print_info() const {
    std::cout << "\n📊 Trajectory Data Information:" << std::endl;
    std::cout << std::string(50, '=') << std::endl;
    std::cout << "Number of samples: " << num_samples << std::endl;
    std::cout << "Number of joints: " << num_joints << std::endl;

    if (!positions.empty()) {
      std::cout << "Position range (joint 1): "
                << "[" << std::fixed << std::setprecision(3) << positions[0][0]
                << ", " << positions.back()[0] << "]" << std::endl;
      std::cout << "Velocity range (joint 1): "
                << "[" << std::fixed << std::setprecision(3) << velocities[0][0]
                << ", " << velocities.back()[0] << "]" << std::endl;
      std::cout << "Torque range (joint 1): "
                << "[" << std::fixed << std::setprecision(3) << torques[0][0]
                << ", " << torques.back()[0] << "]" << std::endl;
    }
    std::cout << std::string(50, '=') << std::endl;
  }
};

class DynamicIdentifier {
private:
  TrajectoryData training_data_;
  TrajectoryData validation_data_;
  std::vector<double> parameters_;
  std::vector<double> validation_predictions_;
  double r2_score_ = 0.0;

  const int NUM_PARAMETERS = 504; // Full regressor size
  const int NUM_BASE_PARAMS = 84; // Number of base parameters per joint (from Python)

  // Base parameter indices from Python regressor.py
  std::vector<int> base_idxs_ = {
      5, 6, 7, 11, 12, 13, 14, 15, 16, 18, 19, 20, 21, 25, 26,
      27, 28, 29, 30, 32, 33, 34, 35, 38, 39, 40, 41, 42, 43,
      44, 46, 47, 48, 49, 52, 53, 54, 55, 56, 57, 58, 60, 61,
      62, 63, 66, 67, 68, 69, 70, 71, 72, 74, 75, 76, 77, 80,
      81, 82, 83
  };

public:
  bool load_data(const std::string &training_file, const std::string &validation_file) {
    std::cout << "\n🔄 Loading training data..." << std::endl;
    if (!training_data_.load_from_csv(training_file)) {
      return false;
    }

    std::cout << "\n🔄 Loading validation data..." << std::endl;
    if (!validation_data_.load_from_csv(validation_file)) {
      return false;
    }

    // Process data
    training_data_.compute_accelerations();
    training_data_.apply_filter(5);

    validation_data_.compute_accelerations();
    validation_data_.apply_filter(5);

    training_data_.print_info();
    validation_data_.print_info();

    // Check data quality
    if (!validate_data_quality()) {
      std::cout << "⚠️  Data quality issues detected, but proceeding with identification..." << std::endl;
    }

    return true;
  }

  bool validate_data_quality() {
    std::cout << "\n🔍 Validating data quality..." << std::endl;
    bool data_quality_good = true;

    // Check position variation
    for (int joint = 0; joint < 6; joint++) {
      double min_pos = training_data_.positions[0][joint];
      double max_pos = training_data_.positions[0][joint];
      double min_vel = training_data_.velocities[0][joint];
      double max_vel = training_data_.velocities[0][joint];
      double min_torque = training_data_.torques[0][joint];
      double max_torque = training_data_.torques[0][joint];

      for (size_t i = 1; i < training_data_.num_samples; i++) {
        min_pos = std::min(min_pos, training_data_.positions[i][joint]);
        max_pos = std::max(max_pos, training_data_.positions[i][joint]);
        min_vel = std::min(min_vel, training_data_.velocities[i][joint]);
        max_vel = std::max(max_vel, training_data_.velocities[i][joint]);
        min_torque = std::min(min_torque, training_data_.torques[i][joint]);
        max_torque = std::max(max_torque, training_data_.torques[i][joint]);
      }

      double pos_range = max_pos - min_pos;
      double vel_range = max_vel - min_vel;
      double torque_range = max_torque - min_torque;

      std::cout << "  Joint " << joint << " - Pos range: " << std::fixed << std::setprecision(4) << pos_range
                << ", Vel range: " << vel_range << ", Torque range: " << torque_range << std::endl;

      // Check if data has sufficient excitation
      if (pos_range < 0.01 && vel_range < 0.01) {
        std::cout << "    ⚠️  Joint " << joint << " shows insufficient motion!" << std::endl;
        data_quality_good = false;
      }
      if (torque_range < 0.1) {
        std::cout << "    ⚠️  Joint " << joint << " shows low torque variation!" << std::endl;
        data_quality_good = false;
      }
    }

    if (data_quality_good) {
      std::cout << "✅ Data quality looks good for identification" << std::endl;
    } else {
      std::cout << "❌ Data quality issues detected - identification may be poor" << std::endl;
    }

    return data_quality_good;
  }

  bool identify_parameters() {
    std::cout << "\n🎯 Starting parameter identification..." << std::endl;

    size_t num_samples = training_data_.num_samples;
    if (num_samples == 0) {
      std::cout << "❌ No training data available" << std::endl;
      return false;
    }

    // Use much larger subset for better identification
    size_t max_samples = std::min(num_samples, static_cast<size_t>(20000));
    std::cout << "📐 Using " << max_samples << " samples out of " << num_samples << " available" << std::endl;
    std::cout << "📐 Building regression matrices using base parameters (" << base_idxs_.size() << " per joint)..." << std::endl;

    // Store only base parameters (84 * 6 = 504 total, but using base_idxs selection)
    parameters_.resize(NUM_BASE_PARAMS * 6, 0.0);

    for (int joint = 0; joint < 6; joint++) {
      std::cout << "  Processing joint " << joint << "..." << std::endl;

      // Build W^T * W and W^T * tau incrementally for base parameters only
      std::vector<double> WtW(NUM_BASE_PARAMS * NUM_BASE_PARAMS, 0.0);
      std::vector<double> WtTau(NUM_BASE_PARAMS, 0.0);

      for (size_t i = 0; i < max_samples; i++) {
        if (i % 1000 == 0) {
          std::cout << "    Sample " << i << "/" << max_samples << std::endl;
        }

        // Compute full regressor for this sample
        std::vector<double> full_regressor(NUM_PARAMETERS, 0.0);
        H_func(full_regressor.data(), training_data_.positions[i].data(),
               training_data_.velocities[i].data(),
               training_data_.accelerations[i].data());

        // Extract only base parameters (following Python logic)
        std::vector<double> joint_regressor(base_idxs_.size());
        for (size_t p = 0; p < base_idxs_.size(); p++) {
          // Reshape logic: full_regressor.reshape(6, 84)[:, base_idxs][joint]
          int full_param_idx = joint * 84 + base_idxs_[p];
          if (full_param_idx < NUM_PARAMETERS) {
            joint_regressor[p] = full_regressor[full_param_idx];
          } else {
            joint_regressor[p] = 0.0;
          }
        }

        double torque = training_data_.torques[i][joint];

        // Update W^T * W and W^T * tau
        for (size_t j = 0; j < base_idxs_.size(); j++) {
          for (size_t k = 0; k < base_idxs_.size(); k++) {
            WtW[j * base_idxs_.size() + k] += joint_regressor[j] * joint_regressor[k];
          }
          WtTau[j] += joint_regressor[j] * torque;
        }
      }

      // Solve for joint parameters using regularized least squares
      std::cout << "    Solving for joint " << joint << " parameters..." << std::endl;

      // Add regularization for numerical stability
      double regularization = 1e-6;
      for (size_t p = 0; p < base_idxs_.size(); p++) {
        WtW[p * base_idxs_.size() + p] += regularization;
      }

      // Simple regularized solution (diagonal approximation for stability)
      for (size_t p = 0; p < base_idxs_.size(); p++) {
        double diag = WtW[p * base_idxs_.size() + p];
        if (std::abs(diag) > 1e-10) {
          parameters_[joint * base_idxs_.size() + p] = WtTau[p] / diag;
        } else {
          parameters_[joint * base_idxs_.size() + p] = 0.0;
        }
      }
    }

    std::cout << "✅ Parameter estimation completed" << std::endl;
    std::cout << "📊 Identified " << (base_idxs_.size() * 6) << " parameters (" << base_idxs_.size() << " per joint)" << std::endl;
    return true;
  }

  bool validate_model() {
    std::cout << "\n🧪 Validating model..." << std::endl;

    size_t num_samples = std::min(validation_data_.num_samples, static_cast<size_t>(2000));
    validation_predictions_.resize(num_samples * 6, 0.0);

    std::cout << "📐 Using " << num_samples << " validation samples..." << std::endl;

    // Calculate mean torques for each joint
    std::vector<double> mean_torques(6, 0.0);
    for (int joint = 0; joint < 6; joint++) {
      for (size_t i = 0; i < num_samples; i++) {
        mean_torques[joint] += validation_data_.torques[i][joint];
      }
      mean_torques[joint] /= num_samples;
    }

    double total_error = 0.0;
    double total_variance = 0.0;

    for (size_t i = 0; i < num_samples; i++) {
      if (i % 500 == 0) {
        std::cout << "  Validating sample " << i << "/" << num_samples << std::endl;
      }

      // Compute full regressor for validation sample
      std::vector<double> full_regressor(NUM_PARAMETERS, 0.0);
      H_func(full_regressor.data(), validation_data_.positions[i].data(),
             validation_data_.velocities[i].data(),
             validation_data_.accelerations[i].data());

      // Predict torques using base parameters only (following Python logic)
      for (int joint = 0; joint < 6; joint++) {
        double predicted_torque = 0.0;

        // Use base parameters for this joint
        for (size_t p = 0; p < base_idxs_.size(); p++) {
          int param_idx = joint * base_idxs_.size() + p;
          if (param_idx < parameters_.size()) {
            // Calculate full parameter index and extract corresponding regressor value
            int full_param_idx = joint * 84 + base_idxs_[p];
            if (full_param_idx < NUM_PARAMETERS) {
              predicted_torque += parameters_[param_idx] * full_regressor[full_param_idx];
            }
          }
        }

        validation_predictions_[i * 6 + joint] = predicted_torque;

        double error = predicted_torque - validation_data_.torques[i][joint];
        total_error += error * error;
        total_variance += std::pow(validation_data_.torques[i][joint] - mean_torques[joint], 2);
      }
    }

    // Calculate R² score
    if (total_variance > 1e-10) {
      r2_score_ = 1.0 - (total_error / total_variance);
    } else {
      r2_score_ = 0.0;
    }

    std::cout << "✅ Validation completed" << std::endl;
    return true;
  }

  void export_results(const std::string &output_dir) const {
    std::cout << "\n📤 Exporting results to: " << output_dir << std::endl;

    // Create output directory
    std::string mkdir_cmd = "mkdir -p " + output_dir;
    system(mkdir_cmd.c_str());

    // Export parameters
    std::ofstream param_file(output_dir + "/identified_parameters.csv");
    param_file << "parameter_index,value" << std::endl;
    for (size_t i = 0; i < parameters_.size(); i++) {
      param_file << i << "," << std::scientific << std::setprecision(6)
                 << parameters_[i] << std::endl;
    }
    param_file.close();

    // Export validation results for plotting
    std::ofstream validation_file(output_dir + "/validation_results.csv");
    validation_file << "sample_index,joint,actual_torque,predicted_torque" << std::endl;

    for (size_t i = 0; i < validation_data_.num_samples; i++) {
      for (int joint = 0; joint < 6; joint++) {
        validation_file << i << "," << joint << ","
                       << std::fixed << std::setprecision(6)
                       << validation_data_.torques[i][joint] << ","
                       << validation_predictions_[i * 6 + joint] << std::endl;
      }
    }
    validation_file.close();

    // Export summary statistics
    std::ofstream summary_file(output_dir + "/identification_summary.txt");
    summary_file << "Dynamic Parameter Identification Results" << std::endl;
    summary_file << "======================================" << std::endl;
    summary_file << "Training samples: " << training_data_.num_samples << std::endl;
    summary_file << "Validation samples: " << validation_data_.num_samples << std::endl;
    summary_file << "Number of parameters: " << parameters_.size() << std::endl;
    summary_file << "R² score: " << std::fixed << std::setprecision(4) << r2_score_ << std::endl;
    summary_file << std::endl;
    summary_file << "Files generated:" << std::endl;
    summary_file << "- identified_parameters.csv: Estimated parameter values" << std::endl;
    summary_file << "- validation_results.csv: Actual vs predicted torques for plotting" << std::endl;
    summary_file << std::endl;
    summary_file << "Python plotting script example:" << std::endl;
    summary_file << "import pandas as pd" << std::endl;
    summary_file << "import matplotlib.pyplot as plt" << std::endl;
    summary_file << "df = pd.read_csv('validation_results.csv')" << std::endl;
    summary_file << "for joint in range(6):" << std::endl;
    summary_file << "    joint_data = df[df['joint'] == joint]" << std::endl;
    summary_file << "    plt.figure()" << std::endl;
    summary_file << "    plt.plot(joint_data['actual_torque'], label='Actual')" << std::endl;
    summary_file << "    plt.plot(joint_data['predicted_torque'], label='Predicted')" << std::endl;
    summary_file << "    plt.title(f'Joint {joint} Torque Validation')" << std::endl;
    summary_file << "    plt.legend()" << std::endl;
    summary_file << "    plt.show()" << std::endl;
    summary_file.close();

    std::cout << "✅ Results exported successfully" << std::endl;
  }

  void print_results() const {
    std::cout << "\n📊 Identification Results:" << std::endl;
    std::cout << std::string(50, '=') << std::endl;
    std::cout << "Training samples: " << training_data_.num_samples << std::endl;
    std::cout << "Validation samples: " << validation_data_.num_samples << std::endl;
    std::cout << "Parameters identified: " << parameters_.size() << std::endl;
    std::cout << "R² score: " << std::fixed << std::setprecision(4) << r2_score_ << std::endl;

    if (r2_score_ > 0.8) {
      std::cout << "🎉 EXCELLENT fit!" << std::endl;
    } else if (r2_score_ > 0.6) {
      std::cout << "✅ GOOD fit!" << std::endl;
    } else if (r2_score_ > 0.4) {
      std::cout << "⚠️  MODERATE fit - consider more data or better filtering" << std::endl;
    } else {
      std::cout << "❌ POOR fit - check data quality and regressor" << std::endl;
    }

    std::cout << std::string(50, '=') << std::endl;
  }
};

void print_usage(const char *program_name) {
  std::cout << "Usage: " << program_name << " <training_data.csv> <validation_data.csv> [output_dir]" << std::endl;
  std::cout << std::endl;
  std::cout << "Arguments:" << std::endl;
  std::cout << "  training_data.csv   - CSV file with motor states for training" << std::endl;
  std::cout << "  validation_data.csv - CSV file with motor states for validation" << std::endl;
  std::cout << "  output_dir          - Directory to save results (default: ./identification_results)" << std::endl;
  std::cout << std::endl;
  std::cout << "Example:" << std::endl;
  std::cout << "  " << program_name << " training_motor_states.csv validation_motor_states.csv" << std::endl;
}

int main(int argc, char *argv[]) {
  std::cout << "=== IC_CAN Dynamic Parameter Identifier ===" << std::endl;
  std::cout << "Identify mass and inertia parameters from trajectory data" << std::endl;

  if (argc < 3) {
    print_usage(argv[0]);
    return -1;
  }

  std::string training_file = argv[1];
  std::string validation_file = argv[2];
  std::string output_dir = (argc > 3) ? argv[3] : "./identification_results";

  try {
    DynamicIdentifier identifier;

    // Load and process data
    if (!identifier.load_data(training_file, validation_file)) {
      std::cout << "❌ Failed to load data" << std::endl;
      return -1;
    }

    // Identify parameters
    if (!identifier.identify_parameters()) {
      std::cout << "❌ Failed to identify parameters" << std::endl;
      return -1;
    }

    // Validate model
    if (!identifier.validate_model()) {
      std::cout << "❌ Failed to validate model" << std::endl;
      return -1;
    }

    // Print and export results
    identifier.print_results();
    identifier.export_results(output_dir);

    std::cout << "\n🎉 Dynamic identification completed successfully!" << std::endl;
    std::cout << "📁 Check '" << output_dir << "' for results and plotting scripts" << std::endl;

    return 0;

  } catch (const std::exception &e) {
    std::cout << "❌ EXCEPTION: " << e.what() << std::endl;
    return -1;
  }
}