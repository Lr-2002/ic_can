/**
 * @file torque_predictor_config.h
 * @brief Torque Predictor Configuration
 *
 * Configuration for switching between different torque prediction methods:
 * - PURE_C_MATLAB: Uses MATLAB-generated M, C, G matrices
 * - REGRESSOR_BASED: Uses regressor-based approach with friction
 *
 * @author IC_CAN Team
 * @date 2025-10-15
 */

#ifndef IC_CAN_TORQUE_PREDICTOR_CONFIG_H
#define IC_CAN_TORQUE_PREDICTOR_CONFIG_H

namespace ic_can {

/**
 * @brief Torque prediction methods enumeration
 */
enum class TorquePredictionMethod {
    PURE_C_MATLAB = 0,    ///< Use pure C MATLAB M, C, G matrix functions
    REGRESSOR_BASED = 1   ///< Use regressor-based approach with friction
};

/**
 * @brief Get current torque prediction method
 *
 * This can be set via:
 * - Environment variable: IC_CAN_TORQUE_METHOD
 * - CMake build option: TORQUE_PREDICTION_METHOD
 * - Runtime configuration file
 *
 * @return Current torque prediction method
 */
TorquePredictionMethod get_torque_prediction_method();

/**
 * @brief Set torque prediction method
 * @param method The desired torque prediction method
 */
void set_torque_prediction_method(TorquePredictionMethod method);

/**
 * @brief Get method name as string
 * @param method Torque prediction method
 * @return String representation of method name
 */
const char* get_torque_method_name(TorquePredictionMethod method);

/**
 * @brief Check if a method is available
 * @param method Torque prediction method to check
 * @return true if method is available and initialized
 */
bool is_torque_method_available(TorquePredictionMethod method);

} // namespace ic_can

#endif // IC_CAN_TORQUE_PREDICTOR_CONFIG_H