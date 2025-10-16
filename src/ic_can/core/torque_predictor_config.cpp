/**
 * @file torque_predictor_config.cpp
 * @brief Torque Predictor Configuration Implementation
 *
 * Implementation for switching between different torque prediction methods.
 *
 * @author IC_CAN Team
 * @date 2025-10-15
 */

#include "ic_can/core/torque_predictor_config.h"
#include <cstdlib>
#include <iostream>

namespace ic_can {

// Default method
static TorquePredictionMethod current_method_ = TorquePredictionMethod::PURE_C_MATLAB;

TorquePredictionMethod get_torque_prediction_method() {
    // Check environment variable first
    const char* env_method = std::getenv("IC_CAN_TORQUE_METHOD");
    if (env_method) {
        std::string method_str(env_method);
        if (method_str == "PURE_C_MATLAB" || method_str == "0") {
            return TorquePredictionMethod::PURE_C_MATLAB;
        } else if (method_str == "REGRESSOR_BASED" || method_str == "1") {
            return TorquePredictionMethod::REGRESSOR_BASED;
        }
    }

    // Return current method
    return current_method_;
}

void set_torque_prediction_method(TorquePredictionMethod method) {
    current_method_ = method;
    std::cout << "🔧 Torque prediction method set to: " << get_torque_method_name(method) << std::endl;
}

const char* get_torque_method_name(TorquePredictionMethod method) {
    switch (method) {
        case TorquePredictionMethod::PURE_C_MATLAB:
            return "Pure C MATLAB (M,C,G matrices)";
        case TorquePredictionMethod::REGRESSOR_BASED:
            return "Regressor-Based (with friction)";
        default:
            return "Unknown";
    }
}

bool is_torque_method_available(TorquePredictionMethod method) {
    // For now, assume both methods are available if their respective
    // classes can be initialized. This can be enhanced with runtime checks.
    switch (method) {
        case TorquePredictionMethod::PURE_C_MATLAB:
        case TorquePredictionMethod::REGRESSOR_BASED:
            return true;
        default:
            return false;
    }
}

} // namespace ic_can