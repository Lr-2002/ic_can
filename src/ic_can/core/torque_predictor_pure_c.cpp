/**
 * @file torque_predictor_pure_c.cpp
 * @brief Pure C Torque Prediction System Implementation
 *
 * Uses pure C MATLAB-generated functions for accurate torque prediction:
 * - M_mtrx_fcn: Mass/inertia matrix computation
 * - C_mtrx_fcn: Coriolis/centrifugal matrix computation
 * - G_vctr_fcn: Gravity vector computation
 *
 * Torque equation: τ = M(q)·ddq + C(q,dq)·dq + G(q)
 *
 * @author IC_CAN Team
 * @date 2025-10-15
 */

#include "ic_can/core/torque_predictor_pure_c.h"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <array>

// Include the pure C MATLAB generated functions
#include "regressor/lib/M_mtrx_fcn/M_mtrx_fcn.h"
#include "regressor/lib/C_mtrx_fcn/C_mtrx_fcn.h"
#include "regressor/lib/G_vctr_fcn/G_vctr_fcn.h"

#include "regressor/lib/M_mtrx_fcn/M_mtrx_fcn_initialize.h"
#include "regressor/lib/C_mtrx_fcn/C_mtrx_fcn_initialize.h"
#include "regressor/lib/G_vctr_fcn/G_vctr_fcn_initialize.h"

#include "regressor/lib/M_mtrx_fcn/M_mtrx_fcn_terminate.h"
#include "regressor/lib/C_mtrx_fcn/C_mtrx_fcn_terminate.h"
#include "regressor/lib/G_vctr_fcn/G_vctr_fcn_terminate.h"

namespace ic_can {

TorquePredictorPureC::TorquePredictorPureC() : initialized_(false) {
    std::cout << "🔧 Initializing TorquePredictorPureC with pure C MATLAB functions..." << std::endl;

    // Initialize all MATLAB Coder functions
    M_mtrx_fcn_initialize();
    C_mtrx_fcn_initialize();
    G_vctr_fcn_initialize();

    // Load identified parameters - check both old and new locations
    std::string param_file = "../lib/matlab/identified_pi_full.txt";
    if (!std::ifstream(param_file).good()) {
        param_file = "regressor/identified_pi_full.txt"; // fallback to old path
    }

    if (load_parameters(param_file)) {
        initialized_ = true;
        std::cout << "✅ TorquePredictorPureC initialized successfully" << std::endl;
    } else {
        std::cout << "❌ Failed to initialize TorquePredictorPureC" << std::endl;
    }
}

TorquePredictorPureC::~TorquePredictorPureC() {
    if (initialized_) {
        std::cout << "🔧 Terminating MATLAB Coder runtime..." << std::endl;
        M_mtrx_fcn_terminate();
        C_mtrx_fcn_terminate();
        G_vctr_fcn_terminate();
    }
}

bool TorquePredictorPureC::is_initialized() const {
    return initialized_;
}

bool TorquePredictorPureC::load_parameters(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cout << "❌ Failed to open parameter file: " << filename << std::endl;
        return false;
    }

    for (int i = 0; i < NUM_PARAMS; i++) {
        if (!(file >> params_[i])) {
            std::cout << "❌ Failed to read parameter " << i << std::endl;
            return false;
        }
    }

    std::cout << "✅ Loaded " << NUM_PARAMS << " identified parameters" << std::endl;
    return true;
}

bool TorquePredictorPureC::predict_torques(const double q[6], const double dq[6], const double ddq[6],
                                          double M_torque[6], double C_torque[6], double G_torque[6],
                                          double total_torque[6]) {
    if (!initialized_) {
        std::cout << "❌ TorquePredictorPureC not initialized" << std::endl;
        return false;
    }

    try {
        // Compute matrices using pure C MATLAB functions
        double M_matrix[MATRIX_SIZE];  // 6x6 mass matrix (flattened)
        double C_matrix[MATRIX_SIZE];  // 6x6 Coriolis matrix (flattened)
        double G_vector[NUM_JOINTS];   // 6x1 gravity vector

        // Call the pure C functions
        M_mtrx_fcn(q, params_, M_matrix);
        C_mtrx_fcn(q, dq, params_, C_matrix);
        G_vctr_fcn(q, params_, G_vector);

        // Compute torque components
        for (int i = 0; i < NUM_JOINTS; i++) {
            // Mass/inertia torque: M * ddq
            M_torque[i] = 0.0;
            for (int j = 0; j < NUM_JOINTS; j++) {
                M_torque[i] += M_matrix[i * NUM_JOINTS + j] * ddq[j];
            }

            // Coriolis torque: C * dq
            C_torque[i] = 0.0;
            for (int j = 0; j < NUM_JOINTS; j++) {
                C_torque[i] += C_matrix[i * NUM_JOINTS + j] * dq[j];
            }

            // Gravity torque
            G_torque[i] = G_vector[i];

            // Total torque
            total_torque[i] = M_torque[i] + C_torque[i] + G_torque[i];
        }

        return true;

    } catch (const std::exception& e) {
        std::cout << "❌ Exception in torque prediction: " << e.what() << std::endl;
        return false;
    }
}

bool TorquePredictorPureC::predict_total_torque(const double q[6], const double dq[6], const double ddq[6],
                                                double total_torque[6]) {
    double M_torque[6], C_torque[6], G_torque[6];
    return predict_torques(q, dq, ddq, M_torque, C_torque, G_torque, total_torque);
}

bool TorquePredictorPureC::predict_gravity_torque(const double q[6], double gravity_torque[6]) {
    if (!initialized_) {
        std::cout << "❌ TorquePredictorPureC not initialized" << std::endl;
        return false;
    }

    // Call the pure C function for gravity only
    G_vctr_fcn(q, params_, gravity_torque);
    return true;
}

void TorquePredictorPureC::print_torque_breakdown(const double q[6], const double dq[6], const double ddq[6]) {
    double M_torque[6], C_torque[6], G_torque[6], total_torque[6];

    if (!predict_torques(q, dq, ddq, M_torque, C_torque, G_torque, total_torque)) {
        std::cout << "❌ Failed to predict torques" << std::endl;
        return;
    }

    std::cout << "\n📊 Torque Component Breakdown (Pure C MATLAB Functions):" << std::endl;
    std::cout << std::string(85, '=') << std::endl;
    std::cout << "Joint |   Mass Torque   | Coriolis Torque |  Gravity Torque |   Total Torque" << std::endl;
    std::cout << std::string(85, '-') << std::endl;

    for (int i = 0; i < NUM_JOINTS; i++) {
        std::cout << std::setw(5) << i << " | "
                  << std::fixed << std::setprecision(4) << std::setw(13) << M_torque[i] << " | "
                  << std::fixed << std::setprecision(4) << std::setw(15) << C_torque[i] << " | "
                  << std::fixed << std::setprecision(4) << std::setw(15) << G_torque[i] << " | "
                  << std::fixed << std::setprecision(4) << std::setw(14) << total_torque[i] << std::endl;
    }
    std::cout << std::string(85, '=') << std::endl;

    // Print additional information
    double total_m_torque = 0, total_c_torque = 0, total_g_torque = 0, total = 0;
    for (int i = 0; i < NUM_JOINTS; i++) {
        total_m_torque += std::abs(M_torque[i]);
        total_c_torque += std::abs(C_torque[i]);
        total_g_torque += std::abs(G_torque[i]);
        total += std::abs(total_torque[i]);
    }

    std::cout << "📈 Magnitude Summary:" << std::endl;
    std::cout << "  Mass contribution:    " << std::fixed << std::setprecision(2)
              << (total_m_torque/total*100) << "%" << std::endl;
    std::cout << "  Coriolis contribution: " << std::fixed << std::setprecision(2)
              << (total_c_torque/total*100) << "%" << std::endl;
    std::cout << "  Gravity contribution: " << std::fixed << std::setprecision(2)
              << (total_g_torque/total*100) << "%" << std::endl;
    std::cout << std::string(85, '=') << std::endl;
}

} // namespace ic_can