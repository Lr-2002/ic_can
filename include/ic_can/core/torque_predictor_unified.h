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
#include "ic_can/core/torque_predictor_pinocchio.h"
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
    std::unique_ptr<TorquePredictorPinocchio> pinocchio_predictor_;
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
     * @param q Joint positions (size depends on DOF of loaded model)
     * @param dq Joint velocities (size depends on DOF of loaded model)
     * @param ddq Joint accelerations (size depends on DOF of loaded model)
     * @param M_torque Output mass/inertia torque component (size depends on DOF)
     * @param C_torque Output Coriolis torque component (size depends on DOF)
     * @param G_torque Output gravity torque component (size depends on DOF)
     * @param total_torque Output total predicted torque (size depends on DOF)
     * @return true if prediction successful
     */
    bool predict_torques(const double* q, const double* dq, const double* ddq,
                        double* M_torque, double* C_torque, double* G_torque,
                        double* total_torque);

    /**
     * @brief Predict total torque for given joint state
     * @param q Joint positions (size depends on DOF of loaded model)
     * @param dq Joint velocities (size depends on DOF of loaded model)
     * @param ddq Joint accelerations (size depends on DOF of loaded model)
     * @param total_torque Output total predicted torque (size depends on DOF)
     * @return true if prediction successful
     */
    bool predict_total_torque(const double* q, const double* dq, const double* ddq,
                             double* total_torque);

    /**
     * @brief Predict only gravity compensation torques
     * @param q Joint positions (size depends on DOF of loaded model)
     * @param gravity_torque Output gravity torques (size depends on DOF)
     * @return true if prediction successful
     */
    bool predict_gravity_torque(const double* q, double* gravity_torque);

    /**
     * @brief Print detailed torque breakdown for analysis
     * @param q Joint positions (size depends on DOF of loaded model)
     * @param dq Joint velocities (size depends on DOF of loaded model)
     * @param ddq Joint accelerations (size depends on DOF of loaded model)
     */
    void print_torque_breakdown(const double* q, const double* dq, const double* ddq);

    /**
     * @brief Get number of degrees of freedom
     * @return DOF count from underlying predictor
     */
    int get_dof() const;

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