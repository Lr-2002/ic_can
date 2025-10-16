/**
 * @file torque_predictor_regressor.h
 * @brief Regressor-Based Torque Prediction System Header
 *
 * Uses regressor-based approach for torque prediction:
 * - regressorWithMotorDynamics: computes Y(q, dq, ddq) matrix
 * - frictionRegressor: computes friction regressor
 * - Torque equation: τ = [y_d, y_f] * [pi_b, pi_fr]
 *
 * @author IC_CAN Team
 * @date 2025-10-15
 */

#ifndef IC_CAN_TORQUE_PREDICTOR_REGRESSOR_H
#define IC_CAN_TORQUE_PREDICTOR_REGRESSOR_H

#include <string>
#include <vector>

// Forward declarations for regressor MATLAB functions
extern "C" {
    void regressorWithMotorDynamics(const double q[6], const double qd[6],
                                   const double q2d[6], double Y[396]);
    void frictionRegressor(const double qd_fltrd[6], double frctn[108]);

    void regressorWithMotorDynamics_initialize();
    void frictionRegressor_initialize();

    void regressorWithMotorDynamics_terminate();
    void frictionRegressor_terminate();
}

namespace ic_can {

/**
 * @brief Regressor-Based Torque Prediction System
 *
 * This class provides torque prediction using the regressor-based approach.
 * It computes the dynamic torques using: τ = [y_d, y_f] * [pi_b, pi_fr]
 * where y_d is the dynamics regressor and y_f is the friction regressor.
 */
class TorquePredictorRegressor {
private:
    static constexpr int NUM_JOINTS = 6;           // Number of arm joints
    static constexpr int NUM_BASE_PARAMS = 46;     // Number of base parameters
    static constexpr int NUM_FRICTION_PARAMS = 18; // Number of friction parameters
    static constexpr int Y_SIZE = 396;             // Size of Y matrix
    static constexpr int FRICTION_SIZE = 108;      // Size of friction regressor
    static constexpr int PERMUTATION_ROWS = 66;    // Permutation matrix rows
    static constexpr int PERMUTATION_COLS = 66;    // Permutation matrix columns

    // Parameters
    std::vector<double> pi_b_;                    // Base dynamic parameters [46]
    std::vector<double> pi_fr_;                   // Friction parameters [18]
    std::vector<double> permutation_matrix_;      // Permutation matrix [66 x 396]

    bool initialized_;                            // Initialization status

    /**
     * @brief Load parameters from file
     * @param filename Path to parameter file
     * @param output Vector to store loaded parameters
     * @param expected_count Expected number of parameters
     * @return true if parameters loaded successfully
     */
    bool load_parameters(const std::string& filename, std::vector<double>& output, int expected_count);

    /**
     * @brief Load permutation matrix from file
     * @param filename Path to permutation matrix file
     * @return true if matrix loaded successfully
     */
    bool load_permutation_matrix(const std::string& filename);

    /**
     * @brief Extract base parameters columns from permutation matrix
     * @param col_start Starting column (usually 0)
     * @param col_count Number of columns to extract (usually n_base)
     * @return Extracted submatrix
     */
    std::vector<double> extract_permutation_columns(int col_start, int col_count);

public:
    /**
     * @brief Constructor - initializes MATLAB Coder runtime and loads parameters
     */
    TorquePredictorRegressor();

    /**
     * @brief Destructor - terminates MATLAB Coder runtime
     */
    ~TorquePredictorRegressor();

    /**
     * @brief Check if the predictor is properly initialized
     * @return true if initialized successfully
     */
    bool is_initialized() const;

    /**
     * @brief Predict torque components for given joint state
     * @param q Joint positions [6]
     * @param dq Joint velocities [6]
     * @param ddq Joint accelerations [6]
     * @param M_torque Output mass/inertia torque component [6]
     * @param C_torque Output Coriolis torque component [6]
     * @param G_torque Output gravity torque component [6]
     * @param F_torque Output friction torque component [6]
     * @param total_torque Output total predicted torque [6]
     * @return true if prediction successful
     */
    bool predict_torques(const double q[6], const double dq[6], const double ddq[6],
                        double M_torque[6], double C_torque[6], double G_torque[6],
                        double F_torque[6], double total_torque[6]);

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

    /**
     * @brief Get method name for display
     * @return String identifier for this method
     */
    std::string get_method_name() const { return "Regressor-Based"; }
};

} // namespace ic_can

#endif // IC_CAN_TORQUE_PREDICTOR_REGRESSOR_H