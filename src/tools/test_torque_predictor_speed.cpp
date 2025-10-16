/**
 * @file test_torque_predictor_speed.cpp
 * @brief Performance Test for Torque Prediction System
 *
 * Tests the computation frequency of the pure C MATLAB functions
 * with random joint states.
 */

#include <iostream>
#include <fstream>
#include <iomanip>
#include <vector>
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

class TorquePredictorPerfTest {
private:
    double params[66];  // Pre-identified parameters
    bool initialized;
    std::mt19937 rng;
    std::uniform_real_distribution<double> q_dist;    // Joint positions: -π to π
    std::uniform_real_distribution<double> dq_dist;   // Velocities: -2 to 2 rad/s
    std::uniform_real_distribution<double> ddq_dist;  // Accelerations: -5 to 5 rad/s²

public:
    bool is_initialized() const { return initialized; }

    TorquePredictorPerfTest() : initialized(false), rng(42),
        q_dist(-3.14159, 3.14159), dq_dist(-2.0, 2.0), ddq_dist(-5.0, 5.0) {

        // Initialize MATLAB Coder functions
        M_mtrx_fcn_initialize();
        C_mtrx_fcn_initialize();
        G_vctr_fcn_initialize();

        // Load identified parameters
        if (load_parameters("regressor/identified_pi_full.txt")) {
            initialized = true;
        }
    }

    ~TorquePredictorPerfTest() {
        if (initialized) {
            M_mtrx_fcn_terminate();
            C_mtrx_fcn_terminate();
            G_vctr_fcn_terminate();
        }
    }

    bool load_parameters(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cout << "❌ Failed to open parameter file: " << filename << std::endl;
            return false;
        }

        for (int i = 0; i < 66; i++) {
            if (!(file >> params[i])) {
                std::cout << "❌ Failed to read parameter " << i << std::endl;
                return false;
            }
        }

        std::cout << "✅ Loaded " << 66 << " identified parameters" << std::endl;
        return true;
    }

    bool predict_torques(const double q[6], const double dq[6], const double ddq[6],
                        double total_torque[6]) {
        if (!initialized) return false;

        // Compute matrices using pure C MATLAB functions
        double M_matrix[36];  // 6x6 mass matrix (flattened)
        double C_matrix[36];  // 6x6 Coriolis matrix (flattened)
        double G_vector[6];   // 6x1 gravity vector

        // Call the pure C functions
        M_mtrx_fcn(q, params, M_matrix);
        C_mtrx_fcn(q, dq, params, C_matrix);
        G_vctr_fcn(q, params, G_vector);

        // Compute total torque: τ = M*ddq + C*dq + G
        for (int i = 0; i < 6; i++) {
            total_torque[i] = G_vector[i];  // Start with gravity

            // Add M*ddq contribution
            for (int j = 0; j < 6; j++) {
                total_torque[i] += M_matrix[i * 6 + j] * ddq[j];
            }

            // Add C*dq contribution
            for (int j = 0; j < 6; j++) {
                total_torque[i] += C_matrix[i * 6 + j] * dq[j];
            }
        }

        return true;
    }

    void run_performance_test(int num_iterations = 10000) {
        if (!initialized) {
            std::cout << "❌ Torque predictor not initialized" << std::endl;
            return;
        }

        std::cout << "\n🚀 Performance Test: " << num_iterations << " iterations" << std::endl;
        std::cout << std::string(80, '=') << std::endl;

        // Generate random test data
        std::vector<std::vector<double>> q_data(num_iterations);
        std::vector<std::vector<double>> dq_data(num_iterations);
        std::vector<std::vector<double>> ddq_data(num_iterations);
        std::vector<std::vector<double>> torque_results(num_iterations);

        for (int i = 0; i < num_iterations; i++) {
            q_data[i].resize(6);
            dq_data[i].resize(6);
            ddq_data[i].resize(6);
            torque_results[i].resize(6);

            for (int j = 0; j < 6; j++) {
                q_data[i][j] = q_dist(rng);
                dq_data[i][j] = dq_dist(rng);
                ddq_data[i][j] = ddq_dist(rng);
            }
        }

        // Performance measurement
        auto start_time = std::chrono::high_resolution_clock::now();

        for (int i = 0; i < num_iterations; i++) {
            predict_torques(q_data[i].data(), dq_data[i].data(), ddq_data[i].data(), torque_results[i].data());
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

        // Show sample results
        std::cout << "\n📋 Sample Results (first 3 iterations):" << std::endl;
        std::cout << std::string(80, '-') << std::endl;
        std::cout << "Iter |      q[0] |    dq[0] |   ddq[0] |     τ[0] |    q[1] |    dq[1] |   ddq[1] |     τ[1]" << std::endl;
        std::cout << std::string(80, '-') << std::endl;

        for (int i = 0; i < std::min(3, num_iterations); i++) {
            std::cout << std::setw(4) << i << " | "
                      << std::fixed << std::setprecision(3) << std::setw(8) << q_data[i][0] << " | "
                      << std::fixed << std::setprecision(3) << std::setw(7) << dq_data[i][0] << " | "
                      << std::fixed << std::setprecision(3) << std::setw(7) << ddq_data[i][0] << " | "
                      << std::fixed << std::setprecision(3) << std::setw(7) << torque_results[i][0] << " | "
                      << std::fixed << std::setprecision(3) << std::setw(7) << q_data[i][1] << " | "
                      << std::fixed << std::setprecision(3) << std::setw(7) << dq_data[i][1] << " | "
                      << std::fixed << std::setprecision(3) << std::setw(7) << ddq_data[i][1] << " | "
                      << std::fixed << std::setprecision(3) << std::setw(7) << torque_results[i][1] << std::endl;
        }

        std::cout << std::string(80, '=') << std::endl;
    }

    void run_frequency_sustained_test(int duration_seconds = 10) {
        if (!initialized) {
            std::cout << "❌ Torque predictor not initialized" << std::endl;
            return;
        }

        std::cout << "\n⏱️  Sustained Frequency Test: " << duration_seconds << " seconds" << std::endl;
        std::cout << std::string(80, '=') << std::endl;

        double q[6], dq[6], ddq[6], total_torque[6];
        int iteration_count = 0;

        auto start_time = std::chrono::high_resolution_clock::now();
        auto end_time = start_time + std::chrono::seconds(duration_seconds);

        while (std::chrono::high_resolution_clock::now() < end_time) {
            // Generate random joint state
            for (int i = 0; i < 6; i++) {
                q[i] = q_dist(rng);
                dq[i] = dq_dist(rng);
                ddq[i] = ddq_dist(rng);
            }

            // Predict torques
            predict_torques(q, dq, ddq, total_torque);
            iteration_count++;
        }

        auto actual_end_time = std::chrono::high_resolution_clock::now();
        auto actual_duration = std::chrono::duration_cast<std::chrono::milliseconds>(actual_end_time - start_time);

        double actual_frequency = iteration_count / (actual_duration.count() / 1000.0);

        std::cout << "📊 Sustained Performance Results:" << std::endl;
        std::cout << "  Test duration:    " << actual_duration.count() << " ms" << std::endl;
        std::cout << "  Iterations:       " << iteration_count << std::endl;
        std::cout << "  Sustained freq:    " << std::fixed << std::setprecision(1) << actual_frequency << " Hz" << std::endl;
        std::cout << "  Iterations/s:     " << std::fixed << std::setprecision(0) << iteration_count * 1000.0 / actual_duration.count() << std::endl;
        std::cout << std::string(80, '=') << std::endl;
    }
};

int main(int argc, char *argv[]) {
    std::cout << "=== Torque Predictor Performance Test ===" << std::endl;
    std::cout << "Testing computation frequency with random joint states" << std::endl;

    TorquePredictorPerfTest tester;

    if (!tester.is_initialized()) {
        std::cout << "❌ Failed to initialize torque predictor" << std::endl;
        return -1;
    }

    // Parse command line arguments
    int quick_iterations = 10000;
    int sustained_duration = 10;

    if (argc > 1) {
        quick_iterations = std::atoi(argv[1]);
    }
    if (argc > 2) {
        sustained_duration = std::atoi(argv[2]);
    }

    // Run performance tests
    tester.run_performance_test(quick_iterations);
    tester.run_frequency_sustained_test(sustained_duration);

    std::cout << "\n✅ Performance testing completed!" << std::endl;
    return 0;
}