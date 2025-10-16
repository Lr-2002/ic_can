/**
 * @file torque_predictor_pure_c.h
 * @brief Pure C Torque Prediction System Header
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

#ifndef IC_CAN_TORQUE_PREDICTOR_PURE_C_H
#define IC_CAN_TORQUE_PREDICTOR_PURE_C_H

#include <string>

// Forward declarations for pure C MATLAB functions
extern "C" {
    void M_mtrx_fcn(const double q[6], const double params[66], double M_matrix[36]);
    void C_mtrx_fcn(const double q[6], const double dq[6], const double params[66], double C_matrix[36]);
    void G_vctr_fcn(const double q[6], const double params[66], double G_vector[6]);

    void M_mtrx_fcn_initialize();
    void C_mtrx_fcn_initialize();
    void G_vctr_fcn_initialize();

    void M_mtrx_fcn_terminate();
    void C_mtrx_fcn_terminate();
    void G_vctr_fcn_terminate();
}

namespace ic_can {

/**
 * @brief Pure C Torque Prediction System
 *
 * This class provides torque prediction using pure C MATLAB-generated functions.
 * It computes the dynamic torques required for robot control using the
 * standard rigid body dynamics equation: τ = M(q)·ddq + C(q,dq)·dq + G(q)
 */
class TorquePredictorPureC {
private:
    static constexpr int NUM_PARAMS = 66;  // Number of identified parameters
    static constexpr int NUM_JOINTS = 6;   // Number of arm joints (excluding gripper)
    static constexpr int MATRIX_SIZE = 36; // 6x6 matrix flattened size

    double params_[NUM_PARAMS];  // Pre-identified dynamic parameters
    bool initialized_;           // Initialization status

public:
    /**
     * @brief Constructor - initializes MATLAB Coder runtime and loads parameters
     */
    TorquePredictorPureC();

    /**
     * @brief Destructor - terminates MATLAB Coder runtime
     */
    ~TorquePredictorPureC();

    /**
     * @brief Check if the predictor is properly initialized
     * @return true if initialized successfully
     */
    bool is_initialized() const;

    /**
     * @brief Load identified parameters from file
     * @param filename Path to parameter file
     * @return true if parameters loaded successfully
     */
    bool load_parameters(const std::string& filename);

    /**
     * @brief Predict torque components for given joint state
     * @param q Joint positions [6]
     * @param dq Joint velocities [6]
     * @param ddq Joint accelerations [6]
     * @param M_torque Output mass/inertia torque component [6]
     * @param C_torque Output Coriolis torque component [6]
     * @param G_torque Output gravity torque component [6]
     * @param total_torque Output total predicted torque [6]
     * @return true if prediction successful
     */
    bool predict_torques(const double q[6], const double dq[6], const double ddq[6],
                        double M_torque[6], double C_torque[6], double G_torque[6],
                        double total_torque[6]);

    /**
     * @brief Predict total torque for given joint state
     * @param q Joint positions [6]
     * @param dq Joint velocities [6]
     * @param ddq Joint accelerations [6]
     * @param total_torque Output total predicted torque [6]
     * @return true if prediction successful
     */
    bool predict_total_torque(const double q[6], const double dq[6], const double ddq[6],
                             double total_torque[6]);

    /**
     * @brief Predict only gravity compensation torques
     * @param q Joint positions [6]
     * @param gravity_torque Output gravity torques [6]
     * @return true if prediction successful
     */
    bool predict_gravity_torque(const double q[6], double gravity_torque[6]);

    /**
     * @brief Print detailed torque breakdown for analysis
     * @param q Joint positions [6]
     * @param dq Joint velocities [6]
     * @param ddq Joint accelerations [6]
     */
    void print_torque_breakdown(const double q[6], const double dq[6], const double ddq[6]);
};

} // namespace ic_can

#endif // IC_CAN_TORQUE_PREDICTOR_PURE_C_H