/**
 * @file torque_predictor_pure_c.cpp
 * @brief Torque Prediction System Using Pure C MATLAB Functions
 *
 * Uses the pure C generated MATLAB functions for accurate torque prediction:
 * - M_mtrx_fcn: Mass/inertia matrix computation
 * - C_mtrx_fcn: Coriolis/centrifugal matrix computation
 * - G_vctr_fcn: Gravity vector computation
 *
 * Torque equation: τ = M(q)·ddq + C(q,dq)·dq + G(q)
 */

#include <iostream>
#include <fstream>
#include <iomanip>
#include <vector>
#include <chrono>

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

class TorquePredictorPureC {
private:
    double params[66];  // Pre-identified parameters
    bool initialized;

public:
    bool is_initialized() const { return initialized; }
    TorquePredictorPureC() : initialized(false) {
        std::cout << "🔧 Initializing MATLAB Coder runtime..." << std::endl;

        // Initialize all MATLAB Coder functions
        M_mtrx_fcn_initialize();
        C_mtrx_fcn_initialize();
        G_vctr_fcn_initialize();

        // Load identified parameters
        if (load_parameters("regressor/identified_pi_full.txt")) {
            initialized = true;
            std::cout << "✅ TorquePredictorPureC initialized successfully" << std::endl;
        } else {
            std::cout << "❌ Failed to initialize TorquePredictorPureC" << std::endl;
        }
    }

    ~TorquePredictorPureC() {
        if (initialized) {
            std::cout << "🔧 Terminating MATLAB Coder runtime..." << std::endl;
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

    /**
     * @brief Predict torque components for given joint state
     * @param q Joint positions [6]
     * @param dq Joint velocities [6]
     * @param ddq Joint accelerations [6]
     * @param M_torque Output mass/inertia torque component [6]
     * @param C_torque Output Coriolis torque component [6]
     * @param G_torque Output gravity torque component [6]
     * @param total_torque Output total predicted torque [6]
     * @return true if successful
     */
    bool predict_torques(const double q[6], const double dq[6], const double ddq[6],
                        double M_torque[6], double C_torque[6], double G_torque[6],
                        double total_torque[6]) {

        if (!initialized) {
            std::cout << "❌ TorquePredictorPureC not initialized" << std::endl;
            return false;
        }

        try {
            // Compute matrices using pure C MATLAB functions
            double M_matrix[36];  // 6x6 mass matrix (flattened)
            double C_matrix[36];  // 6x6 Coriolis matrix (flattened)
            double G_vector[6];   // 6x1 gravity vector

            // Call the pure C functions
            M_mtrx_fcn(q, params, M_matrix);
            C_mtrx_fcn(q, dq, params, C_matrix);
            G_vctr_fcn(q, params, G_vector);

            // Compute torque components
            for (int i = 0; i < 6; i++) {
                // Mass/inertia torque: M * ddq
                M_torque[i] = 0.0;
                for (int j = 0; j < 6; j++) {
                    M_torque[i] += M_matrix[i * 6 + j] * ddq[j];
                }

                // Coriolis torque: C * dq
                C_torque[i] = 0.0;
                for (int j = 0; j < 6; j++) {
                    C_torque[i] += C_matrix[i * 6 + j] * dq[j];
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

    void print_torque_breakdown(const double q[6], const double dq[6], const double ddq[6]) {
        double M_torque[6], C_torque[6], G_torque[6], total_torque[6];

        if (!predict_torques(q, dq, ddq, M_torque, C_torque, G_torque, total_torque)) {
            std::cout << "❌ Failed to predict torques" << std::endl;
            return;
        }

        std::cout << "\n📊 Torque Component Breakdown (Pure C MATLAB Functions):" << std::endl;
        std::cout << std::string(85, '=') << std::endl;
        std::cout << "Joint |   Mass Torque   | Coriolis Torque |  Gravity Torque |   Total Torque" << std::endl;
        std::cout << std::string(85, '-') << std::endl;

        for (int i = 0; i < 6; i++) {
            std::cout << std::setw(5) << i << " | "
                      << std::fixed << std::setprecision(4) << std::setw(13) << M_torque[i] << " | "
                      << std::fixed << std::setprecision(4) << std::setw(15) << C_torque[i] << " | "
                      << std::fixed << std::setprecision(4) << std::setw(15) << G_torque[i] << " | "
                      << std::fixed << std::setprecision(4) << std::setw(14) << total_torque[i] << std::endl;
        }
        std::cout << std::string(85, '=') << std::endl;

        // Print additional information
        double total_m_torque = 0, total_c_torque = 0, total_g_torque = 0, total = 0;
        for (int i = 0; i < 6; i++) {
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

    bool test_with_sample_data() {
        std::cout << "\n🧪 Testing with sample data..." << std::endl;

        // Test with some sample joint states
        double q1[6] = {0.0, 0.1, -0.2, 0.3, -0.1, 0.0};
        double dq1[6] = {0.1, 0.2, -0.1, 0.0, 0.15, -0.05};
        double ddq1[6] = {0.5, -0.3, 0.2, 0.1, -0.4, 0.0};

        std::cout << "\nTest 1: Near-zero configuration with moderate velocities" << std::endl;
        print_torque_breakdown(q1, dq1, ddq1);

        double q2[6] = {0.5, 0.8, -0.6, 0.4, -0.3, 0.2};
        double dq2[6] = {1.0, 0.5, -0.8, 0.3, 0.6, -0.4};
        double ddq2[6] = {2.0, -1.5, 1.0, -0.5, 1.2, -0.8};

        std::cout << "\nTest 2: Extended configuration with high dynamics" << std::endl;
        print_torque_breakdown(q2, dq2, ddq2);

        return true;
    }
};

void print_usage(const char *program_name) {
    std::cout << "Usage: " << program_name << " [mode]" << std::endl;
    std::cout << std::endl;
    std::cout << "Modes:" << std::endl;
    std::cout << "  interactive  - Interactive mode to test different joint states" << std::endl;
    std::cout << "  test        - Run built-in test cases" << std::endl;
    std::cout << "  csv <file>  - Process trajectory from CSV file" << std::endl;
    std::cout << std::endl;
    std::cout << "Example:" << std::endl;
    std::cout << "  " << program_name << " test" << std::endl;
}

int main(int argc, char *argv[]) {
    std::cout << "=== IC_CAN Torque Prediction System (Pure C MATLAB Functions) ===" << std::endl;
    std::cout << "Using pre-identified dynamic parameters and pure C MATLAB functions" << std::endl;

    if (argc < 2) {
        print_usage(argv[0]);
        return -1;
    }

    std::string mode = argv[1];
    TorquePredictorPureC predictor;

    if (!predictor.is_initialized()) {
        std::cout << "❌ Failed to initialize torque predictor" << std::endl;
        return -1;
    }

    if (mode == "interactive") {
        std::cout << "\n🎮 Interactive Mode - Enter joint states to see torque breakdown" << std::endl;
        std::cout << "Format: q1 q2 q3 q4 q5 q6 dq1 dq2 dq3 dq4 dq5 dq6 ddq1 ddq2 ddq3 ddq4 ddq5 ddq6" << std::endl;
        std::cout << "Type 'quit' to exit" << std::endl;

        while (true) {
            std::cout << "\n📍 Enter joint state (18 values): ";
            std::string line;
            std::getline(std::cin, line);

            if (line == "quit" || line == "exit") {
                break;
            }

            std::stringstream ss(line);
            double q[6], dq[6], ddq[6];
            bool valid = true;

            for (int i = 0; i < 6; i++) {
                if (!(ss >> q[i])) valid = false;
            }
            for (int i = 0; i < 6; i++) {
                if (!(ss >> dq[i])) valid = false;
            }
            for (int i = 0; i < 6; i++) {
                if (!(ss >> ddq[i])) valid = false;
            }

            if (!valid) {
                std::cout << "❌ Invalid input. Please enter 18 numeric values." << std::endl;
                continue;
            }

            predictor.print_torque_breakdown(q, dq, ddq);
        }

    } else if (mode == "test") {
        std::cout << "\n🧪 Running test cases..." << std::endl;
        predictor.test_with_sample_data();

    } else if (mode == "csv" && argc >= 3) {
        std::string csv_file = argv[2];
        std::cout << "\n📂 Processing trajectory from: " << csv_file << std::endl;
        std::cout << "CSV processing not yet implemented" << std::endl;

    } else {
        std::cout << "❌ Invalid mode or missing arguments" << std::endl;
        print_usage(argv[0]);
        return -1;
    }

    std::cout << "\n✅ Torque prediction completed!" << std::endl;
    return 0;
}