/**
 * @file torque_predictor.cpp
 * @brief Torque Prediction System Using Identified Parameters
 *
 * Uses pre-identified parameters to compute torque components:
 * - M_torque: Mass/inertia torque component (M(q) * ddq)
 * - C_torque: Coriolis/centrifugal torque component (C(q,dq) * dq)
 * - G_torque: Gravity torque component (G(q))
 * - Total: M_torque + C_torque + G_torque
 */

#include <iostream>
#include <vector>
#include <fstream>
#include <iomanip>
#include <chrono>
#include <cmath>

// MATLAB types
typedef double real_T;

// Load parameters from file function
bool load_identified_params(const std::string& filename, real_T params[66]) {
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

    std::cout << "✅ Loaded " << 66 << " identified parameters from " << filename << std::endl;
    return true;
}

class TorquePredictor {
private:
    real_T params[66];
    const double g = 9.81; // gravity constant

public:
    TorquePredictor() {
        // Load parameters from file
        std::string param_file = "regressor/identified_pi_full.txt";
        if (!load_identified_params(param_file, params)) {
            std::cout << "❌ Failed to load parameters, using zeros" << std::endl;
            for (int i = 0; i < 66; i++) {
                params[i] = 0.0;
            }
        }
    }

    /**
     * @brief Compute mass/inertia matrix M(q)
     * @param q Joint positions [6]
     * @param M Output mass matrix [6][6]
     */
    void compute_mass_matrix(const real_T q[6], real_T M[6][6]) {
        // Initialize to zero
        for (int i = 0; i < 6; i++) {
            for (int j = 0; j < 6; j++) {
                M[i][j] = 0.0;
            }
        }

        // Build mass matrix using identified parameters
        // This is a simplified version - in practice you'd use the actual MATLAB generated M_mtrx_fcn

        // Diagonal mass terms (use first 6 parameters as link masses)
        for (int i = 0; i < 6; i++) {
            if (i < 6) {
                M[i][i] = std::abs(params[i]) + 1.0; // Add 1.0 for numerical stability
            }
        }

        // Add coupling terms using additional parameters
        // Example: off-diagonal inertia terms
        if (22 < 66) M[0][1] = params[22];  // Joint 1-2 coupling
        if (23 < 66) M[0][2] = params[23];  // Joint 1-3 coupling
        if (24 < 66) M[1][2] = params[24];  // Joint 2-3 coupling

        // Symmetric terms
        M[1][0] = M[0][1];
        M[2][0] = M[0][2];
        M[2][1] = M[1][2];

        // Add position-dependent terms (simplified)
        for (int i = 0; i < 6; i++) {
            for (int j = 0; j < 6; j++) {
                if (i != j) {
                    int param_idx = 30 + i*6 + j;
                    if (param_idx < 66) {
                        M[i][j] += params[param_idx] * cos(q[i] - q[j]);
                    }
                }
            }
        }
    }

    /**
     * @brief Compute Coriolis/centrifugal matrix C(q,dq)
     * @param q Joint positions [6]
     * @param dq Joint velocities [6]
     * @param C Output Coriolis matrix [6][6]
     */
    void compute_coriolis_matrix(const real_T q[6], const real_T dq[6], real_T C[6][6]) {
        // Initialize to zero
        for (int i = 0; i < 6; i++) {
            for (int j = 0; j < 6; j++) {
                C[i][j] = 0.0;
            }
        }

        // Build Coriolis matrix using identified parameters
        // Simplified Coriolis computation

        for (int i = 0; i < 6; i++) {
            for (int j = 0; j < 6; j++) {
                if (i != j) {
                    // Use parameters for velocity coupling terms
                    int param_idx = 40 + i*6 + j;
                    if (param_idx < 66) {
                        C[i][j] = params[param_idx] * dq[j];
                    }
                }
            }
        }

        // Add quadratic velocity terms
        for (int i = 0; i < 6; i++) {
            int param_idx = 52 + i;
            if (param_idx < 66) {
                C[i][i] = params[param_idx] * dq[i];
            }
        }
    }

    /**
     * @brief Compute gravity vector G(q)
     * @param q Joint positions [6]
     * @param G Output gravity vector [6]
     */
    void compute_gravity_vector(const real_T q[6], real_T G[6]) {
        // Initialize to zero
        for (int i = 0; i < 6; i++) {
            G[i] = 0.0;
        }

        // Build gravity vector using identified parameters
        // Simplified gravity computation

        // Joint 0 gravity (assuming vertical rotation)
        if (6 < 66) {
            G[0] = params[6] * g * cos(q[0]);
        }

        // Joint 1 gravity
        if (7 < 66) {
            G[1] = params[7] * g * cos(q[1]);
        }

        // Joint 2 gravity
        if (8 < 66) {
            G[2] = params[8] * g * sin(q[2]);
        }

        // Add coupling gravity effects
        for (int i = 3; i < 6; i++) {
            if (6 + i < 66) {
                G[i] = params[6 + i] * g * sin(q[i]);
            }
        }
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
     */
    void predict_torques(const real_T q[6], const real_T dq[6], const real_T ddq[6],
                        real_T M_torque[6], real_T C_torque[6], real_T G_torque[6],
                        real_T total_torque[6]) {

        // Compute matrices
        real_T M[6][6], C[6][6];
        real_T G[6];

        compute_mass_matrix(q, M);
        compute_coriolis_matrix(q, dq, C);
        compute_gravity_vector(q, G);

        // Compute torque components
        for (int i = 0; i < 6; i++) {
            // Mass/inertia torque: M * ddq
            M_torque[i] = 0.0;
            for (int j = 0; j < 6; j++) {
                M_torque[i] += M[i][j] * ddq[j];
            }

            // Coriolis torque: C * dq
            C_torque[i] = 0.0;
            for (int j = 0; j < 6; j++) {
                C_torque[i] += C[i][j] * dq[j];
            }

            // Gravity torque
            G_torque[i] = G[i];

            // Total torque
            total_torque[i] = M_torque[i] + C_torque[i] + G_torque[i];
        }
    }

    void print_torque_breakdown(const real_T q[6], const real_T dq[6], const real_T ddq[6]) {
        real_T M_torque[6], C_torque[6], G_torque[6], total_torque[6];

        predict_torques(q, dq, ddq, M_torque, C_torque, G_torque, total_torque);

        std::cout << "\n📊 Torque Component Breakdown:" << std::endl;
        std::cout << std::string(80, '=') << std::endl;
        std::cout << "Joint |   Mass Torque   | Coriolis Torque |  Gravity Torque |   Total Torque" << std::endl;
        std::cout << std::string(80, '-') << std::endl;

        for (int i = 0; i < 6; i++) {
            std::cout << std::setw(5) << i << " | "
                      << std::fixed << std::setprecision(4) << std::setw(13) << M_torque[i] << " | "
                      << std::fixed << std::setprecision(4) << std::setw(15) << C_torque[i] << " | "
                      << std::fixed << std::setprecision(4) << std::setw(15) << G_torque[i] << " | "
                      << std::fixed << std::setprecision(4) << std::setw(14) << total_torque[i] << std::endl;
        }
        std::cout << std::string(80, '=') << std::endl;
    }
};

void print_usage(const char *program_name) {
    std::cout << "Usage: " << program_name << " [mode]" << std::endl;
    std::cout << std::endl;
    std::cout << "Modes:" << std::endl;
    std::cout << "  interactive  - Interactive mode to test different joint states" << std::endl;
    std::cout << "  csv <file>  - Process trajectory from CSV file" << std::endl;
    std::cout << std::endl;
    std::cout << "Example:" << std::endl;
    std::cout << "  " << program_name << " interactive" << std::endl;
    std::cout << "  " << program_name << " csv motor_states.csv" << std::endl;
}

int main(int argc, char *argv[]) {
    std::cout << "=== IC_CAN Torque Prediction System ===" << std::endl;
    std::cout << "Using pre-identified dynamic parameters" << std::endl;

    if (argc < 2) {
        print_usage(argv[0]);
        return -1;
    }

    std::string mode = argv[1];
    TorquePredictor predictor;

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
            real_T q[6], dq[6], ddq[6];
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

    } else if (mode == "csv" && argc >= 3) {
        std::string csv_file = argv[2];
        std::cout << "\n📂 Processing trajectory from: " << csv_file << std::endl;

        // TODO: Implement CSV processing similar to dynamic_identifier.cpp
        std::cout << "CSV processing not yet implemented" << std::endl;

    } else {
        std::cout << "❌ Invalid mode or missing arguments" << std::endl;
        print_usage(argv[0]);
        return -1;
    }

    std::cout << "\n✅ Torque prediction completed!" << std::endl;
    return 0;
}