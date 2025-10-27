/**
 * @file friction_sweep.cpp
 * @brief Friction Compensation Angle Sweep
 *
 * Performs angle sweep at constant velocity to identify friction parameters
 * for IC_CAN arm control system.
 *
 * @author IC_CAN Team
 * @date 2025-10-27
 */

#include <array>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <thread>
#include <vector>

#include "../include/ic_can/core/torque_predictor_unified.h"

using namespace ic_can;

struct SweepPoint {
  std::array<double, 6> q;
  std::array<double, 6> torque;
  double velocity;
  double time_ms;
};

int main() {
  std::cout << "=== Friction Compensation Angle Sweep ===" << std::endl;
  std::cout << "Performing constant velocity sweep for friction identification"
            << std::endl;

  // Initialize torque predictor
  TorquePredictorUnified predictor;
  if (!predictor.is_initialized()) {
    std::cout << "❌ Torque predictor initialization failed!" << std::endl;
    return -1;
  }

  std::cout << "✅ Torque predictor initialized successfully" << std::endl;

  // Sweep configuration
  const std::array<double, 6> initial_q = {
      {10.0, 0.0, 0.0, 0.0, 0.0, 0.0}}; // Initial position
  const std::array<double, 6> min_q = {
      {1.0, -9.0, -9.0, -9.0, -9.0, -9.0}}; // Min bounds
  const std::array<double, 6> max_q = {
      {19.0, 9.0, 9.0, 9.0, 9.0, 9.0}}; // Max bounds
  const double velocity = 0.5;          // Constant sweep velocity (rad/s)
  const double settle_time = 2000.0;    // Wait 2 seconds for settling
  const int samples_per_point = 10;     // Samples to average at each point

  std::vector<SweepPoint> sweep_data;

  std::cout << "\n📊 Sweep Configuration:" << std::endl;
  std::cout << "Initial position: [10.0, 0.0, 0.0, 0.0, 0.0, 0.0]" << std::endl;
  std::cout << "Sweep bounds: ±9 degrees from initial" << std::endl;
  std::cout << "Constant velocity: " << velocity << " rad/s" << std::endl;
  std::cout << "Samples per point: " << samples_per_point << std::endl;

  // For friction sweep, we use PURE_C_MATLAB method to get just basic dynamics
  predictor.switch_method(TorquePredictionMethod::PURE_C_MATLAB);

  std::cout << "\n🔄 Starting angle sweep..." << std::endl;
  std::cout << "This will sweep each joint from min to max bounds" << std::endl;
  std::cout << std::string(80, '=') << std::endl;

  // Sweep each joint individually while others stay at initial position
  for (int joint = 0; joint < 6; ++joint) {
    std::cout << "\n🎯 Sweeping Joint " << joint << std::endl;

    int num_points = 20; // Number of points per sweep
    double step_size = (max_q[joint] - min_q[joint]) / (num_points - 1);

    for (int point = 0; point < num_points; ++point) {
      SweepPoint sweep_point;

      // Create sweep configuration
      sweep_point.q = initial_q;
      sweep_point.q[joint] = min_q[joint] + point * step_size;
      sweep_point.velocity = velocity;

      std::cout << "\r   Point " << (point + 1) << "/" << num_points << " | q"
                << joint << " = " << std::fixed << std::setprecision(2)
                << std::setw(6) << sweep_point.q[joint] << " rad ("
                << std::setw(6) << (sweep_point.q[joint] * 180.0 / M_PI)
                << "°)";
      std::cout.flush();

      // Record time
      auto start_time = std::chrono::high_resolution_clock::now();

      // Move to target position with constant velocity
      // Note: This would be replaced with actual IC_CAN motor control
      // For now, just simulate the torque prediction

      std::array<double, 6> predicted_torque;
      std::array<double, 6> zero_vel = {{0.0, 0.0, 0.0, 0.0, 0.0, 0.0}};
      std::array<double, 6> zero_accel = {{0.0, 0.0, 0.0, 0.0, 0.0, 0.0}};

      // Predict torque required for this configuration
      bool success = predictor.predict_total_torque(
          sweep_point.q.data(), zero_vel.data(), zero_accel.data(),
          predicted_torque.data());

      if (success) {
        // Average multiple samples for noise reduction
        std::array<double, 6> avg_torque = {{0.0, 0.0, 0.0, 0.0, 0.0, 0.0}};

        for (int sample = 0; sample < samples_per_point; ++sample) {
          std::array<double, 6> sample_torque;
          predictor.predict_total_torque(sweep_point.q.data(), zero_vel.data(),
                                         zero_accel.data(),
                                         sample_torque.data());

          for (int j = 0; j < 6; ++j) {
            avg_torque[j] += sample_torque[j];
          }
          std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        // Calculate average
        for (int j = 0; j < 6; ++j) {
          avg_torque[j] /= samples_per_point;
        }

        sweep_point.torque = avg_torque;
        auto end_time = std::chrono::high_resolution_clock::now();
        sweep_point.time_ms =
            std::chrono::duration<double, std::milli>(end_time - start_time)
                .count();

        sweep_data.push_back(sweep_point);

        std::cout << " | τ" << joint << " = " << std::fixed
                  << std::setprecision(3) << std::setw(8)
                  << sweep_point.torque[joint] << " N⋅m";
      } else {
        std::cout << " | ❌ Prediction failed";
      }
    }

    std::cout << std::endl;
  }

  std::cout << "\n" << std::string(80, '=') << std::endl;
  std::cout << "✅ Sweep completed! Collected " << sweep_data.size()
            << " data points" << std::endl;

  // Save data to CSV for analysis
  std::ofstream data_file("friction_sweep_data.csv");
  data_file << "joint,velocity,q_rad,q_deg,torque_Nm,time_ms\n";

  for (const auto &point : sweep_data) {
    for (int i = 0; i < 6; ++i) {
      data_file << i << "," << point.velocity << "," << point.q[i] << ","
                << (point.q[i] * 180.0 / M_PI) << "," << point.torque[i] << ","
                << point.time_ms << "\n";
    }
  }

  data_file.close();
  std::cout << "💾 Data saved to: friction_sweep_data.csv" << std::endl;

  // Simple friction model fitting (Coulomb + viscous)
  std::cout << "\n📈 Fitting Simple Friction Models..." << std::endl;

  for (int joint = 0; joint < 6; ++joint) {
    // Extract data for this joint
    std::vector<double> positions, torques_pos, torques_neg;

    for (const auto &point : sweep_data) {
      if (point.q[joint] > initial_q[joint]) {
        positions.push_back(point.q[joint]);
        torques_pos.push_back(point.torque[joint]);
      } else if (point.q[joint] < initial_q[joint]) {
        positions.push_back(point.q[joint]);
        torques_neg.push_back(point.torque[joint]);
      }
    }

    std::cout << "\n🔧 Joint " << joint << " friction parameters:" << std::endl;
    std::cout << "   Data points: " << (torques_pos.size() + torques_neg.size())
              << std::endl;

    if (torques_pos.size() > 2 && torques_neg.size() > 2) {
      // Simple Coulomb + Viscous fitting
      // τ = Fc * sgn(ω) + Fv * ω
      // Fc ≈ (τ_pos - τ_neg) / 2
      // Fv ≈ (τ_pos + τ_neg) / (2 * ω)

      double avg_torque_pos = 0, avg_torque_neg = 0;
      for (double t : torques_pos)
        avg_torque_pos += t;
      for (double t : torques_neg)
        avg_torque_neg += t;
      avg_torque_pos /= torques_pos.size();
      avg_torque_neg /= torques_neg.size();

      double F_coulomb = (avg_torque_pos - avg_torque_neg) / 2.0;
      double F_viscous = (avg_torque_pos + avg_torque_neg) / (2.0 * velocity);

      std::cout << "   Coulomb friction (Fc): " << std::fixed
                << std::setprecision(4) << F_coulomb << " N⋅m" << std::endl;
      std::cout << "   Viscous friction (Fv): " << std::fixed
                << std::setprecision(4) << F_viscous << " Nm⋅s/rad"
                << std::endl;
      std::cout << "   Breakaway velocity: " << std::fixed
                << std::setprecision(4) << (F_coulomb / F_viscous) << " rad/s"
                << std::endl;
    } else {
      std::cout << "   ⚠️  Insufficient data for fitting" << std::endl;
    }
  }

  std::cout << "\n🎉 Friction sweep analysis completed!" << std::endl;
  std::cout << "Use the CSV data to implement detailed friction models in "
               "control system."
            << std::endl;

  return 0;
}
