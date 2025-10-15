/**
 * @file regressor_wrapper.cpp
 * @brief Wrapper for MATLAB/Simulink generated regressor functions
 *
 * This file provides a C++ interface to the MATLAB generated functions:
 * - M_mtrx_fcn: Mass/Inertia matrix (6x6)
 * - C_mtrx_fcn: Coriolis/Centrifugal matrix (6x6)
 * - G_vctr_fcn: Gravity vector (6x1)
 */

#include <iostream>
#include <vector>
#include <cmath>

// MATLAB types
typedef double real_T;

// Include the MATLAB generated functions
// We'll need to create standalone versions since these depend on MATLAB runtime

/**
 * @brief Compute mass/inertia matrix
 * @param q Joint positions [6]
 * @param params Dynamic parameters [66]
 * @param M Output mass matrix [36] (6x6 flattened)
 */
void M_mtrx_fcn_standalone(const real_T q[6], const real_T params[66], real_T M[36]) {
    // Initialize to identity as default
    for (int i = 0; i < 6; i++) {
        for (int j = 0; j < 6; j++) {
            M[i*6 + j] = (i == j) ? 1.0 : 0.0;
        }
    }

    // This is a placeholder - you would copy the actual MATLAB generated code here
    // For now, we'll create a simple mass matrix based on the first 6 parameters
    // Assuming first 6 parameters are link masses
    for (int i = 0; i < 6; i++) {
        if (i < 6 && params[i] > 0) {
            M[i*6 + i] = params[i];
        }
    }
}

/**
 * @brief Compute Coriolis/centrifugal matrix
 * @param q Joint positions [6]
 * @param dq Joint velocities [6]
 * @param params Dynamic parameters [66]
 * @param C Output Coriolis matrix [36] (6x6 flattened)
 */
void C_mtrx_fcn_standalone(const real_T q[6], const real_T dq[6], const real_T params[66], real_T C[36]) {
    // Initialize to zero
    for (int i = 0; i < 36; i++) {
        C[i] = 0.0;
    }

    // Placeholder for actual MATLAB generated code
    // Simple Coriolis approximation based on velocities
    for (int i = 0; i < 6; i++) {
        for (int j = 0; j < 6; j++) {
            if (i != j) {
                // Simple velocity coupling term
                C[i*6 + j] = 0.01 * dq[j] * params[12 + i*6 + j]; // Use some parameters for coupling
            }
        }
    }
}

/**
 * @brief Compute gravity vector
 * @param q Joint positions [6]
 * @param params Dynamic parameters [66]
 * @param G Output gravity vector [6]
 */
void G_vctr_fcn_standalone(const real_T q[6], const real_T params[66], real_T G[6]) {
    // Placeholder for actual MATLAB generated code
    // Simple gravity compensation
    double g = 9.81;

    for (int i = 0; i < 6; i++) {
        G[i] = 0.0;
    }

    // Simple gravity for first joint (assuming vertical rotation)
    if (params[0] > 0) {
        G[0] = params[0] * g * cos(q[0]);
    }

    // Add some gravity effects for other joints using additional parameters
    for (int i = 1; i < 3; i++) {  // Assume joints 1-2 are affected by gravity
        if (6 + i < 66) {
            G[i] = params[6 + i] * g * sin(q[i]);
        }
    }
}

/**
 * @brief Compute predicted torques using M, C, G matrices
 * @param q Joint positions [6]
 * @param dq Joint velocities [6]
 * @param ddq Joint accelerations [6]
 * @param params Dynamic parameters [66]
 * @param tau Output predicted torques [6]
 */
void compute_torques(const real_T q[6], const real_T dq[6], const real_T ddq[6],
                    const real_T params[66], real_T tau[6]) {

    // Compute matrices
    real_T M[36], C[36], G[6];

    M_mtrx_fcn_standalone(q, params, M);
    C_mtrx_fcn_standalone(q, dq, params, C);
    G_vctr_fcn_standalone(q, params, G);

    // Compute torques: tau = M*ddq + C*dq + G
    for (int i = 0; i < 6; i++) {
        tau[i] = G[i]; // Start with gravity

        // Add M*ddq contribution
        for (int j = 0; j < 6; j++) {
            tau[i] += M[i*6 + j] * ddq[j];
        }

        // Add C*dq contribution
        for (int j = 0; j < 6; j++) {
            tau[i] += C[i*6 + j] * dq[j];
        }
    }
}

/**
 * @brief Load parameters from the identified parameter file
 * @param filename Path to parameter file
 * @param params Output parameter array [66]
 * @return true if successful
 */
bool load_parameters(const std::string& filename, real_T params[66]) {
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

    std::cout << "✅ Loaded " << 66 << " parameters from " << filename << std::endl;
    return true;
}