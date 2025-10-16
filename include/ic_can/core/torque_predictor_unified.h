/**
 * @file torque_predictor_unified.h
 * @brief Unified Torque Prediction System
 *
 * Provides a unified interface that can switch between different
 * torque prediction methods based on configuration.
 *
 * @author IC_CAN Team
 * @date 2025-10-15
 */

#ifndef IC_CAN_TORQUE_PREDICTOR_UNIFIED_H
#define IC_CAN_TORQUE_PREDICTOR_UNIFIED_H

#include "ic_can/core/torque_predictor_config.h"
#include "ic_can/core/torque_predictor_pure_c.h"
#include "ic_can/core/torque_predictor_regressor.h"
#include <memory>
#include <string>

namespace ic_can {

/**
 * @brief Unified Torque Prediction System
 *
 * This class provides a unified interface that can switch between
 * different torque prediction methods based on runtime configuration.
 */
class TorquePredictorUnified {
private:
    std::unique_ptr<TorquePredictorPureC> pure_c_predictor_;
    std::unique_ptr<TorquePredictorRegressor> regressor_predictor_;
    TorquePredictionMethod current_method_;
    bool initialized_;

public:
    /**
     * @brief Constructor - initializes available predictors
     */
    TorquePredictorUnified();

    /**
     * @brief Destructor
     */
    ~TorquePredictorUnified() = default;

    /**
     * @brief Check if the predictor is properly initialized
     * @return true if at least one method is initialized successfully
     */
    bool is_initialized() const;

    /**
     * @brief Get current torque prediction method
     * @return Current method being used
     */
    TorquePredictionMethod get_current_method() const { return current_method_; }

    /**
     * @brief Switch torque prediction method
     * @param method Desired torque prediction method
     * @return true if switch successful
     */
    bool switch_method(TorquePredictionMethod method);

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

    /**
     * @brief Print available methods and their status
     */
    void print_method_status();

    /**
     * @brief Get method name for display
     * @return String identifier for current method
     */
    std::string get_method_name() const;
};

} // namespace ic_can

#endif // IC_CAN_TORQUE_PREDICTOR_UNIFIED_H