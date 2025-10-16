/**
 * @file torque_predictor_regressor.cpp
 * @brief Regressor-Based Torque Prediction System Implementation
 *
 * Uses regressor-based approach for accurate torque prediction:
 * - regressorWithMotorDynamics: computes Y(q, dq, ddq) matrix
 * - frictionRegressor: computes friction regressor
 * - Torque equation: τ = [y_d, y_f] * [pi_b, pi_fr]
 *
 * @author IC_CAN Team
 * @date 2025-10-15
 */

#include "ic_can/core/torque_predictor_regressor.h"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <algorithm>

// Include the regressor MATLAB generated functions
#include "regressorWithMotorDynamics.h"
#include "frictionRegressor.h"
#include "tmwtypes.h"

#include "regressorWithMotorDynamics_initialize.h"
#include "frictionRegressor_initialize.h"

#include "regressorWithMotorDynamics_terminate.h"
#include "frictionRegressor_terminate.h"

namespace ic_can {

TorquePredictorRegressor::TorquePredictorRegressor() : initialized_(false) {
    std::cout << "🔧 Initializing TorquePredictorRegressor with regressor-based method..." << std::endl;

    // Initialize all MATLAB Coder functions
    regressorWithMotorDynamics_initialize();
    frictionRegressor_initialize();

    // Resize parameter vectors
    pi_b_.resize(NUM_BASE_PARAMS);
    pi_fr_.resize(NUM_FRICTION_PARAMS);
    permutation_matrix_.resize(PERMUTATION_ROWS * PERMUTATION_COLS);

    // Load parameters
    bool success = true;
    success &= load_parameters("../lib/regressor/pi_b.txt", pi_b_, NUM_BASE_PARAMS);
    success &= load_parameters("../lib/regressor/pi_fr.txt", pi_fr_, NUM_FRICTION_PARAMS);
    success &= load_permutation_matrix("../lib/regressor/permutation_matrix.txt");

    if (success) {
        initialized_ = true;
        std::cout << "✅ TorquePredictorRegressor initialized successfully" << std::endl;
        std::cout << "   Base parameters: " << NUM_BASE_PARAMS << std::endl;
        std::cout << "   Friction parameters: " << NUM_FRICTION_PARAMS << std::endl;
    } else {
        std::cout << "❌ Failed to initialize TorquePredictorRegressor" << std::endl;
    }
}

TorquePredictorRegressor::~TorquePredictorRegressor() {
    if (initialized_) {
        std::cout << "🔧 Terminating MATLAB Coder runtime..." << std::endl;
        regressorWithMotorDynamics_terminate();
        frictionRegressor_terminate();
    }
}

bool TorquePredictorRegressor::is_initialized() const {
    return initialized_;
}

bool TorquePredictorRegressor::load_parameters(const std::string& filename, std::vector<double>& output, int expected_count) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cout << "❌ Failed to open parameter file: " << filename << std::endl;
        return false;
    }

    for (int i = 0; i < expected_count; i++) {
        if (!(file >> output[i])) {
            std::cout << "❌ Failed to read parameter " << i << " from " << filename << std::endl;
            return false;
        }
    }

    std::cout << "✅ Loaded " << expected_count << " parameters from " << filename << std::endl;
    return true;
}

bool TorquePredictorRegressor::load_permutation_matrix(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cout << "❌ Failed to open permutation matrix file: " << filename << std::endl;
        return false;
    }

    std::string line;
    int row = 0;
    while (std::getline(file, line) && row < PERMUTATION_ROWS) {
        std::istringstream iss(line);
        for (int col = 0; col < PERMUTATION_COLS; col++) {
            if (!(iss >> permutation_matrix_[row * PERMUTATION_COLS + col])) {
                std::cout << "❌ Failed to read permutation matrix at row " << row << ", col " << col << std::endl;
                return false;
            }
        }
        row++;
    }

    if (row != PERMUTATION_ROWS) {
        std::cout << "❌ Expected " << PERMUTATION_ROWS << " rows, got " << row << std::endl;
        return false;
    }

    std::cout << "✅ Loaded permutation matrix (" << PERMUTATION_ROWS << "x" << PERMUTATION_COLS << ")" << std::endl;
    return true;
}

std::vector<double> TorquePredictorRegressor::extract_permutation_columns(int col_start, int col_count) {
    std::vector<double> result(PERMUTATION_ROWS * col_count);
    for (int row = 0; row < PERMUTATION_ROWS; row++) {
        for (int col = 0; col < col_count; col++) {
            result[row * col_count + col] = permutation_matrix_[row * PERMUTATION_COLS + (col_start + col)];
        }
    }
    return result;
}

bool TorquePredictorRegressor::predict_torques(const double q[6], const double dq[6], const double ddq[6],
                                              double M_torque[6], double C_torque[6], double G_torque[6],
                                              double F_torque[6], double total_torque[6]) {
    if (!initialized_) {
        std::cout << "❌ TorquePredictorRegressor not initialized" << std::endl;
        return false;
    }

    try {
        // Step 1: Compute Y(q, dq, ddq) matrix
        double Y_matrix[Y_SIZE];
        regressorWithMotorDynamics(q, dq, ddq, Y_matrix);

        // Step 2: Use permutation matrix to extract relevant parameters
        // Simplified approach: use the permutation matrix directly since it's 66x66
        double y_d[NUM_JOINTS * NUM_BASE_PARAMS] = {0.0};

        // For each joint, compute the combination of Y elements with permutation matrix
        for (int i = 0; i < NUM_JOINTS; i++) {
            for (int j = 0; j < NUM_BASE_PARAMS; j++) {
                // Y matrix has 66 elements per joint (396/6 = 66)
                for (int k = 0; k < PERMUTATION_COLS; k++) {
                    y_d[i * NUM_BASE_PARAMS + j] += Y_matrix[i * 66 + k] *
                                                   permutation_matrix_[j * PERMUTATION_COLS + k];
                }
            }
        }

        // Step 3: Compute friction regressor
        double friction_regressor[FRICTION_SIZE];
        frictionRegressor(dq, friction_regressor);

        // Step 4: Compute torques
        for (int i = 0; i < NUM_JOINTS; i++) {
            // Dynamics torque: y_d * pi_b
            double dynamics_torque = 0.0;
            for (int j = 0; j < NUM_BASE_PARAMS; j++) {
                dynamics_torque += y_d[i * NUM_BASE_PARAMS + j] * pi_b_[j];
            }

            // Friction torque: y_f * pi_fr
            double friction_torque = 0.0;
            for (int j = 0; j < NUM_FRICTION_PARAMS; j++) {
                friction_torque += friction_regressor[i * NUM_FRICTION_PARAMS + j] * pi_fr_[j];
            }

            // For now, we can't separate M, C, G components easily with regressor method
            // So we'll put all dynamics torque in M_torque and friction in F_torque
            M_torque[i] = dynamics_torque;
            C_torque[i] = 0.0;  // Not easily separable
            G_torque[i] = 0.0;  // Not easily separable
            F_torque[i] = friction_torque;
            total_torque[i] = dynamics_torque + friction_torque;
        }

        return true;

    } catch (const std::exception& e) {
        std::cout << "❌ Exception in torque prediction: " << e.what() << std::endl;
        return false;
    }
}

bool TorquePredictorRegressor::predict_total_torque(const double q[6], const double dq[6], const double ddq[6],
                                                    double total_torque[6]) {
    double M_torque[6], C_torque[6], G_torque[6], F_torque[6];
    return predict_torques(q, dq, ddq, M_torque, C_torque, G_torque, F_torque, total_torque);
}

bool TorquePredictorRegressor::predict_gravity_torque(const double q[6], double gravity_torque[6]) {
    if (!initialized_) {
        std::cout << "❌ TorquePredictorRegressor not initialized" << std::endl;
        return false;
    }

    try {
        // For gravity, we set dq = ddq = 0
        double zero_dq[6] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
        double zero_ddq[6] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0};

        // Compute Y matrix at zero velocity and acceleration
        double Y_matrix[Y_SIZE];
        regressorWithMotorDynamics(q, zero_dq, zero_ddq, Y_matrix);

        // Compute gravity torques using permutation matrix
        for (int i = 0; i < NUM_JOINTS; i++) {
            gravity_torque[i] = 0.0;
            for (int j = 0; j < NUM_BASE_PARAMS; j++) {
                for (int k = 0; k < PERMUTATION_COLS; k++) {
                    gravity_torque[i] += Y_matrix[i * 66 + k] *
                                        permutation_matrix_[j * PERMUTATION_COLS + k] * pi_b_[j];
                }
            }
        }

        return true;

    } catch (const std::exception& e) {
        std::cout << "❌ Exception in gravity torque prediction: " << e.what() << std::endl;
        return false;
    }
}

void TorquePredictorRegressor::print_torque_breakdown(const double q[6], const double dq[6], const double ddq[6]) {
    double M_torque[6], C_torque[6], G_torque[6], F_torque[6], total_torque[6];

    if (!predict_torques(q, dq, ddq, M_torque, C_torque, G_torque, F_torque, total_torque)) {
        std::cout << "❌ Failed to predict torques" << std::endl;
        return;
    }

    std::cout << "\n📊 Torque Component Breakdown (Regressor-Based Method):" << std::endl;
    std::cout << std::string(85, '=') << std::endl;
    std::cout << "Joint | Dynamics Torque | Friction Torque |   Total Torque" << std::endl;
    std::cout << std::string(85, '-') << std::endl;

    for (int i = 0; i < NUM_JOINTS; i++) {
        std::cout << std::setw(5) << i << " | "
                  << std::fixed << std::setprecision(4) << std::setw(16) << M_torque[i] << " | "
                  << std::fixed << std::setprecision(4) << std::setw(15) << F_torque[i] << " | "
                  << std::fixed << std::setprecision(4) << std::setw(14) << total_torque[i] << std::endl;
    }
    std::cout << std::string(85, '=') << std::endl;

    // Print additional information
    double total_dynamics = 0, total_friction = 0, total = 0;
    for (int i = 0; i < NUM_JOINTS; i++) {
        total_dynamics += std::abs(M_torque[i]);
        total_friction += std::abs(F_torque[i]);
        total += std::abs(total_torque[i]);
    }

    std::cout << "📈 Magnitude Summary:" << std::endl;
    std::cout << "  Dynamics contribution: " << std::fixed << std::setprecision(2)
              << (total_dynamics/total*100) << "%" << std::endl;
    std::cout << "  Friction contribution:  " << std::fixed << std::setprecision(2)
              << (total_friction/total*100) << "%" << std::endl;
    std::cout << "🔧 Method: " << get_method_name() << std::endl;
    std::cout << std::string(85, '=') << std::endl;
}

} // namespace ic_can