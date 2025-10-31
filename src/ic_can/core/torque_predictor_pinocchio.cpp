/**
 * @file torque_predictor_pinocchio.cpp
 * @brief Pinocchio-based Torque Prediction System Implementation
 *
 * Implementation for Pinocchio-based torque predictor that reads URDF
 * and calculates precise gravity and Coriolis torque components.
 *
 * @author IC_CAN Team
 * @date 2025-10-27
 */

#include "ic_can/core/torque_predictor_pinocchio.h"
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace ic_can {

// Joint names for the IC arm (6 DOF)
const std::vector<std::string> TorquePredictorPinocchio::joint_names_ = {
    "j1", "j2", "j3", "j4", "j5", "j6"};

// Define static member
const int TorquePredictorPinocchio::DOF = 6;

TorquePredictorPinocchio::TorquePredictorPinocchio(const std::string &urdf_path)
    : urdf_path_(urdf_path), initialized_(false), gravity_(0, 0, -9.81),
      q_config_(DOF), v_config_(DOF), a_config_(DOF) {

  // Initialize default URDF path if not provided
  if (urdf_path_.empty()) {
    // Try to find the URDF in standard locations
    std::vector<std::string> possible_paths = {
        /*"/home/lr-2002/project/InstantCreation/ic_can/urdfs/ic_arm/urdf/ic_arm_cnc.urdf",*/
        "/home/lr-2002/project/InstantCreation/ic_can/urdfs/ic_arm_with_wrist/"
        "urdf/ic_arm_with_wrist.urdf",
        "../urdf/urdf/ic_arm_cnc.urdf",
        "urdf/urdf/ic_arm_cnc.urdf"};

    for (const auto &path : possible_paths) {
        std::cout << "reading " << path << std::endl;
      if (std::filesystem::exists(path)) {
        std::cout << " this path exists " << std::endl;
        urdf_path_ = path;
      
        break;
      }
    }
  }

  std::cout << "🔧 Initializing Pinocchio-based Torque Predictor..."
            << " the urdf is empyt ? " << urdf_path_.empty() << " " << urdf_path_
            << std::endl;
  if (!urdf_path_.empty()) {
    initialize(urdf_path_);
  } else {
    std::cout << "❌ No URDF file found. Please provide a valid path."
              << std::endl;
  }
}

bool TorquePredictorPinocchio::initialize(const std::string &urdf_path) {
  try {
    if (!urdf_path.empty()) {
      urdf_path_ = urdf_path;
    }

    // Check if URDF file exists
    if (!std::filesystem::exists(urdf_path_)) {
      std::cout << "❌ URDF file not found: " << urdf_path_ << std::endl;
      return false;
    }

    std::cout << "📄 Loading URDF: " << urdf_path_ << std::endl;

    // Build model from URDF
    pinocchio::urdf::buildModel(urdf_path_, model_);

    // Create data structure
    data_ = pinocchio::Data(model_);

    // Set gravity vector
    model_.gravity.linear() = gravity_;

    initialized_ = true;
    std::cout << "✅ Pinocchio model initialized successfully!" << std::endl;
    std::cout << "🦾 Model has " << model_.nv << " DOF and " << model_.njoints
              << " joints" << std::endl;

    // Print joint information
    print_model_info();

    return true;

  } catch (const std::exception &e) {
    std::cout << "❌ Failed to initialize Pinocchio model: " << e.what()
              << std::endl;
    initialized_ = false;
    return false;
  }
}

void TorquePredictorPinocchio::set_gravity(const Eigen::Vector3d &gravity) {
  gravity_ = gravity;
  if (initialized_) {
    model_.gravity.linear() = gravity_;
  }
}

void TorquePredictorPinocchio::set_gravity(const double gravity[3]) {
  set_gravity(Eigen::Vector3d(gravity[0], gravity[1], gravity[2]));
}

bool TorquePredictorPinocchio::validate_joint_config(const double q[6],
                                                     const double dq[6],
                                                     const double ddq[6]) {
  // Check for NaN values
  for (int i = 0; i < DOF; ++i) {
    if (std::isnan(q[i]) || std::isnan(dq[i]) || std::isnan(ddq[i])) {
      std::cout << "❌ NaN detected in joint " << i << std::endl;
      return false;
    }
  }
  return true;
}

bool TorquePredictorPinocchio::predict_torques(
    const double q[6], const double dq[6], const double ddq[6],
    double M_torque[6], double C_torque[6], double G_torque[6],
    double total_torque[6]) {
  if (!initialized_) {
    std::cout << "❌ Pinocchio predictor not initialized" << std::endl;
    return false;
  }

  if (!validate_joint_config(q, dq, ddq)) {
    return false;
  }

  try {
    // Convert to Eigen vectors
    for (int i = 0; i < DOF; ++i) {
      q_config_[i] = q[i];
      v_config_[i] = dq[i];
      a_config_[i] = ddq[i];
    }

    // Compute torques using RNEA (Recursive Newton-Euler Algorithm)
    Eigen::VectorXd torques =
        pinocchio::rnea(model_, data_, q_config_, v_config_, a_config_);

    // Compute individual components
    // Mass component (M * ddq)
    Eigen::MatrixXd mass_matrix(DOF, DOF);
    pinocchio::crba(model_, data_, q_config_);
    mass_matrix = data_.M;
    Eigen::VectorXd M_component = mass_matrix * a_config_;

    // Gravity component
    Eigen::VectorXd G_component =
        pinocchio::computeGeneralizedGravity(model_, data_, q_config_);

    // Coriolis component (total - M - G)
    Eigen::VectorXd C_component = torques - M_component - G_component;

    // Copy results to output arrays
    for (int i = 0; i < DOF; ++i) {
      M_torque[i] = M_component[i];
      C_torque[i] = C_component[i];
      G_torque[i] = G_component[i];
      total_torque[i] = torques[i];
    }

    return true;

  } catch (const std::exception &e) {
    std::cout << "❌ Error computing torques: " << e.what() << std::endl;
    return false;
  }
}

bool TorquePredictorPinocchio::predict_total_torque(const double q[6],
                                                    const double dq[6],
                                                    const double ddq[6],
                                                    double total_torque[6]) {
  if (!initialized_) {
    std::cout << "❌ Pinocchio predictor not initialized" << std::endl;
    return false;
  }

  if (!validate_joint_config(q, dq, ddq)) {
    return false;
  }

  try {
    // Convert to Eigen vectors
    for (int i = 0; i < DOF; ++i) {
      q_config_[i] = q[i];
      v_config_[i] = dq[i];
      a_config_[i] = ddq[i];
    }

    // Compute torques using RNEA
    Eigen::VectorXd torques =
        pinocchio::rnea(model_, data_, q_config_, v_config_, a_config_);

    // Copy results to output array
    for (int i = 0; i < DOF; ++i) {
      total_torque[i] = torques[i];
    }

    return true;

  } catch (const std::exception &e) {
    std::cout << "❌ Error computing total torques: " << e.what() << std::endl;
    return false;
  }
}

bool TorquePredictorPinocchio::predict_gravity_torque(
    const double q[6], double gravity_torque[6]) {
  if (!initialized_) {
    std::cout << "❌ Pinocchio predictor not initialized" << std::endl;
    return false;
  }

  try {
    // Convert to Eigen vector
    for (int i = 0; i < DOF; ++i) {
      q_config_[i] = q[i];
    }

    // Compute gravity torques
    Eigen::VectorXd G_torques =
        pinocchio::computeGeneralizedGravity(model_, data_, q_config_);

    // Copy results to output array
    for (int i = 0; i < DOF; ++i) {
      gravity_torque[i] = G_torques[i];
    }

    return true;

  } catch (const std::exception &e) {
    std::cout << "❌ Error computing gravity torques: " << e.what()
              << std::endl;
    return false;
  }
}

bool TorquePredictorPinocchio::compute_mass_matrix(
    const double q[6], Eigen::MatrixXd &mass_matrix) {
  if (!initialized_) {
    std::cout << "❌ Pinocchio predictor not initialized" << std::endl;
    return false;
  }

  try {
    // Convert to Eigen vector
    for (int i = 0; i < DOF; ++i) {
      q_config_[i] = q[i];
    }

    // Compute mass matrix using CRBA (Composite Rigid Body Algorithm)
    pinocchio::crba(model_, data_, q_config_);
    mass_matrix = data_.M;

    // Make symmetric (CRBA returns upper triangular)
    mass_matrix.triangularView<Eigen::StrictlyLower>() =
        mass_matrix.transpose();

    return true;

  } catch (const std::exception &e) {
    std::cout << "❌ Error computing mass matrix: " << e.what() << std::endl;
    return false;
  }
}

bool TorquePredictorPinocchio::compute_coriolis_matrix(
    const double q[6], const double dq[6], Eigen::MatrixXd &coriolis_matrix) {
  if (!initialized_) {
    std::cout << "❌ Pinocchio predictor not initialized" << std::endl;
    return false;
  }

  try {
    // Convert to Eigen vectors
    for (int i = 0; i < DOF; ++i) {
      q_config_[i] = q[i];
      v_config_[i] = dq[i];
    }

    // Compute Coriolis matrix
    pinocchio::computeCoriolisMatrix(model_, data_, q_config_, v_config_);
    coriolis_matrix = data_.C;

    return true;

  } catch (const std::exception &e) {
    std::cout << "❌ Error computing Coriolis matrix: " << e.what()
              << std::endl;
    return false;
  }
}

void TorquePredictorPinocchio::print_torque_breakdown(const double q[6],
                                                      const double dq[6],
                                                      const double ddq[6]) {
  if (!initialized_) {
    std::cout << "❌ Pinocchio predictor not initialized" << std::endl;
    return;
  }

  std::cout << "\n🔧 Pinocchio Torque Analysis Breakdown:" << std::endl;
  std::cout << std::string(60, '=') << std::endl;

  // Predict torques
  double M_torque[6], C_torque[6], G_torque[6], total_torque[6];
  if (predict_torques(q, dq, ddq, M_torque, C_torque, G_torque, total_torque)) {
    std::cout << std::fixed << std::setprecision(4);
    std::cout << std::setw(6) << "Joint" << " | " << std::setw(10) << "M*ddq"
              << " | " << std::setw(10) << "C*dq" << " | " << std::setw(10)
              << "G" << " | " << std::setw(10) << "Total" << std::endl;
    std::cout << std::string(60, '-') << std::endl;

    for (int i = 0; i < DOF; ++i) {
      std::cout << std::setw(6) << ("j" + std::to_string(i + 1)) << " | "
                << std::setw(10) << M_torque[i] << " | " << std::setw(10)
                << C_torque[i] << " | " << std::setw(10) << G_torque[i] << " | "
                << std::setw(10) << total_torque[i] << std::endl;
    }

    // Print magnitude analysis
    double M_mag = 0, C_mag = 0, G_mag = 0, total_mag = 0;
    for (int i = 0; i < DOF; ++i) {
      M_mag += M_torque[i] * M_torque[i];
      C_mag += C_torque[i] * C_torque[i];
      G_mag += G_torque[i] * G_torque[i];
      total_mag += total_torque[i] * total_torque[i];
    }

    std::cout << std::string(60, '-') << std::endl;
    std::cout << "Magnitudes: | " << std::setw(8) << std::sqrt(M_mag) << " | "
              << std::setw(8) << std::sqrt(C_mag) << " | " << std::setw(8)
              << std::sqrt(G_mag) << " | " << std::setw(8)
              << std::sqrt(total_mag) << std::endl;
  }
  std::cout << std::string(60, '=') << std::endl;
}

void TorquePredictorPinocchio::print_model_info() {
  if (!initialized_) {
    std::cout << "❌ Pinocchio predictor not initialized" << std::endl;
    return;
  }

  std::cout << "\n🦾 Pinocchio Robot Model Information:" << std::endl;
  std::cout << std::string(40, '=') << std::endl;
  std::cout << "Number of joints: " << model_.njoints << std::endl;
  std::cout << "Number of DOF: " << model_.nv << std::endl;
  std::cout << "Gravity vector: [" << gravity_.transpose() << "]" << std::endl;

  std::cout << "\nJoint Names:" << std::endl;
  for (int i = 1; i < model_.njoints; ++i) {
    std::cout << "  " << i << ": " << model_.names[i] << std::endl;
  }
  std::cout << std::string(40, '=') << std::endl;
}

bool TorquePredictorPinocchio::is_within_joint_limits(const double q[6]) {
  if (!initialized_) {
    return false;
  }

  // Note: Pinocchio doesn't store joint limits by default
  // This would need to be parsed from URDF if limits checking is required
  // For now, we'll assume joints are within reasonable bounds
  for (int i = 0; i < DOF; ++i) {
    if (std::abs(q[i]) > 6.28) { // More than 2π rad
      std::cout << "⚠️  Joint " << (i + 1) << " position " << q[i]
                << " seems large (> 2π rad)" << std::endl;
      return false;
    }
  }
  return true;
}

bool TorquePredictorPinocchio::get_joint_limits(double lower[6],
                                                double upper[6]) {
  if (!initialized_) {
    return false;
  }

  // Note: Pinocchio doesn't store joint limits by default
  // This would need custom URDF parsing or using a different parser
  // For now, return false to indicate limits are not available
  std::cout << "⚠️  Joint limits not implemented yet (requires URDF parsing)"
            << std::endl;
  return false;
}

} // namespace ic_can
