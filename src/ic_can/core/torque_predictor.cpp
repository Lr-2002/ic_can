/**
 * @file torque_predictor.cpp
 * @brief High-Performance Torque Prediction System Implementation
 *
 * @author IC_CAN Team
 * @date 2025-10-15
 */

#include "ic_can/core/torque_predictor.h"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <chrono>
#include <random>

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

TorquePredictor::TorquePredictor() : initialized_(false) {
    std::cout << "🔧 Initializing TorquePredictor with pure C MATLAB functions..." << std::endl;

    // Initialize all MATLAB Coder functions
    M_mtrx_fcn_initialize();
    C_mtrx_fcn_initialize();
    G_vctr_fcn_initialize();

    // Load identified parameters
    std::string param_file = "regressor/identified_pi_full.txt";
    if (load_parameters(param_file)) {
        initialized_ = true;
        std::cout << "✅ TorquePredictor initialized successfully" << std::endl;
    } else {
        std::cout << "❌ Failed to initialize TorquePredictor" << std::endl;
    }
}

TorquePredictor::~TorquePredictor() {
    if (initialized_) {
        std::cout << "🔧 Terminating MATLAB Coder runtime..." << std::endl;
        M_mtrx_fcn_terminate();
        C_mtrx_fcn_terminate();
        G_vctr_fcn_terminate();
    }
}

bool TorquePredictor::load_parameters(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cout << "❌ Failed to open parameter file: " << filename << std::endl;
        return false;
    }

    for (int i = 0; i < NUM_PARAMS; i++) {
        if (!(file >> parameters_[i])) {
            std::cout << "❌ Failed to read parameter " << i << std::endl;
            return false;
        }
    }

    std::cout << "✅ Loaded " << NUM_PARAMS << " identified parameters" << std::endl;
    return true;
}

void TorquePredictor::compute_total_torque(const double* M_matrix, const double* C_matrix,
                                         const double* G_vector, const double* dq, const double* ddq,
                                         double* total_torque) {
    for (int i = 0; i < NUM_JOINTS; i++) {
        total_torque[i] = G_vector[i];  // Start with gravity

        // Add M*ddq contribution
        for (int j = 0; j < NUM_JOINTS; j++) {
            total_torque[i] += M_matrix[i * NUM_JOINTS + j] * ddq[j];
        }

        // Add C*dq contribution
        for (int j = 0; j < NUM_JOINTS; j++) {
            total_torque[i] += C_matrix[i * NUM_JOINTS + j] * dq[j];
        }
    }
}

bool TorquePredictor::predict_torques(const std::array<double, NUM_JOINTS>& q,
                                     const std::array<double, NUM_JOINTS>& dq,
                                     const std::array<double, NUM_JOINTS>& ddq,
                                     TorqueComponents& torques) {
    if (!initialized_) {
        std::cout << "❌ TorquePredictor not initialized" << std::endl;
        return false;
    }

    try {
        // Compute matrices using pure C MATLAB functions
        std::array<double, MATRIX_SIZE> M_matrix;  // 6x6 mass matrix (flattened)
        std::array<double, MATRIX_SIZE> C_matrix;  // 6x6 Coriolis matrix (flattened)
        std::array<double, NUM_JOINTS> G_vector;   // 6x1 gravity vector

        // Call the pure C functions
        M_mtrx_fcn(q.data(), parameters_.data(), M_matrix.data());
        C_mtrx_fcn(q.data(), dq.data(), parameters_.data(), C_matrix.data());
        G_vctr_fcn(q.data(), parameters_.data(), G_vector.data());

        // Compute torque components
        for (int i = 0; i < NUM_JOINTS; i++) {
            // Mass/inertia torque: M * ddq
            torques.mass_torque[i] = 0.0;
            for (int j = 0; j < NUM_JOINTS; j++) {
                torques.mass_torque[i] += M_matrix[i * NUM_JOINTS + j] * ddq[j];
            }

            // Coriolis torque: C * dq
            torques.coriolis_torque[i] = 0.0;
            for (int j = 0; j < NUM_JOINTS; j++) {
                torques.coriolis_torque[i] += C_matrix[i * NUM_JOINTS + j] * dq[j];
            }

            // Gravity torque
            torques.gravity_torque[i] = G_vector[i];

            // Total torque
            torques.total_torque[i] = torques.mass_torque[i] +
                                     torques.coriolis_torque[i] +
                                     torques.gravity_torque[i];
        }

        return true;

    } catch (const std::exception& e) {
        std::cout << "❌ Exception in torque prediction: " << e.what() << std::endl;
        return false;
    }
}

bool TorquePredictor::predict_total_torque(const double q[NUM_JOINTS],
                                         const double dq[NUM_JOINTS],
                                         const double ddq[NUM_JOINTS],
                                         double total_torque[NUM_JOINTS]) {
    if (!initialized_) return false;

    // Compute matrices using pure C MATLAB functions
    std::array<double, MATRIX_SIZE> M_matrix;  // 6x6 mass matrix (flattened)
    std::array<double, MATRIX_SIZE> C_matrix;  // 6x6 Coriolis matrix (flattened)
    std::array<double, NUM_JOINTS> G_vector;   // 6x1 gravity vector

    // Call the pure C functions
    M_mtrx_fcn(q, parameters_.data(), M_matrix.data());
    C_mtrx_fcn(q, dq, parameters_.data(), C_matrix.data());
    G_vctr_fcn(q, parameters_.data(), G_vector.data());

    // Compute total torque: τ = M*ddq + C*dq + G
    compute_total_torque(M_matrix.data(), C_matrix.data(), G_vector.data(),
                        dq, ddq, total_torque);

    return true;
}

bool TorquePredictor::predict_gravity_torque(const std::array<double, NUM_JOINTS>& q,
                                            std::array<double, NUM_JOINTS>& gravity_torque) {
    if (!initialized_) return false;

    // Call the pure C function for gravity only
    G_vctr_fcn(q.data(), parameters_.data(), gravity_torque.data());
    return true;
}

void TorquePredictor::print_torque_breakdown(const std::array<double, NUM_JOINTS>& q,
                                            const std::array<double, NUM_JOINTS>& dq,
                                            const std::array<double, NUM_JOINTS>& ddq) {
    TorqueComponents torques;

    if (!predict_torques(q, dq, ddq, torques)) {
        std::cout << "❌ Failed to predict torques" << std::endl;
        return;
    }

    std::cout << "\n📊 Torque Component Breakdown (Pure C MATLAB Functions):" << std::endl;
    std::cout << std::string(85, '=') << std::endl;
    std::cout << "Joint |   Mass Torque   | Coriolis Torque |  Gravity Torque |   Total Torque" << std::endl;
    std::cout << std::string(85, '-') << std::endl;

    for (int i = 0; i < NUM_JOINTS; i++) {
        std::cout << std::setw(5) << i << " | "
                  << std::fixed << std::setprecision(4) << std::setw(13) << torques.mass_torque[i] << " | "
                  << std::fixed << std::setprecision(4) << std::setw(15) << torques.coriolis_torque[i] << " | "
                  << std::fixed << std::setprecision(4) << std::setw(15) << torques.gravity_torque[i] << " | "
                  << std::fixed << std::setprecision(4) << std::setw(14) << torques.total_torque[i] << std::endl;
    }
    std::cout << std::string(85, '=') << std::endl;

    // Print additional information
    double total_m_torque = 0, total_c_torque = 0, total_g_torque = 0, total = 0;
    for (int i = 0; i < NUM_JOINTS; i++) {
        total_m_torque += std::abs(torques.mass_torque[i]);
        total_c_torque += std::abs(torques.coriolis_torque[i]);
        total_g_torque += std::abs(torques.gravity_torque[i]);
        total += std::abs(torques.total_torque[i]);
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

double TorquePredictor::run_performance_test(int num_iterations) {
    if (!initialized_) {
        std::cout << "❌ TorquePredictor not initialized" << std::endl;
        return 0.0;
    }

    std::cout << "\n🚀 Performance Test: " << num_iterations << " iterations" << std::endl;
    std::cout << std::string(80, '=') << std::endl;

    // Random number generators
    std::mt19937 rng(42);
    std::uniform_real_distribution<double> q_dist(-3.14159, 3.14159);
    std::uniform_real_distribution<double> dq_dist(-2.0, 2.0);
    std::uniform_real_distribution<double> ddq_dist(-5.0, 5.0);

    // Performance measurement
    auto start_time = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < num_iterations; i++) {
        std::array<double, NUM_JOINTS> q, dq, ddq, total_torque;

        for (int j = 0; j < NUM_JOINTS; j++) {
            q[j] = q_dist(rng);
            dq[j] = dq_dist(rng);
            ddq[j] = ddq_dist(rng);
        }

        predict_total_torque(q.data(), dq.data(), ddq.data(), total_torque.data());
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);

    // Calculate statistics
    double total_time_us = duration.count();
    double avg_time_us = total_time_us / num_iterations;
    double frequency_hz = 1.0 / (avg_time_us / 1000000.0);

    std::cout << "📊 Performance Results:" << std::endl;
    std::cout << "  Total time:      " << std::fixed << std::setprecision(2) << total_time_us << " μs" << std::endl;
    std::cout << "  Average time:    " << std::fixed << std::setprecision(4) << avg_time_us << " μs" << std::endl;
    std::cout << "  Frequency:       " << std::fixed << std::setprecision(1) << frequency_hz << " Hz" << std::endl;

    // Calculate if it meets real-time requirements
    double target_freq = 1000.0; // 1 kHz for control
    if (frequency_hz >= target_freq) {
        std::cout << "  ✅ Meets " << target_freq << " Hz real-time requirement!" << std::endl;
    } else {
        std::cout << "  ⚠️  Below " << target_freq << " Hz - may not meet real-time requirements" << std::endl;
    }
    std::cout << std::string(80, '=') << std::endl;

    return frequency_hz;
}

} // namespace ic_can