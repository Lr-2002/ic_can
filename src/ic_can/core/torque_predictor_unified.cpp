/**
 * @file torque_predictor_unified.cpp
 * @brief Unified Torque Prediction System Implementation
 *
 * Implementation for unified interface that can switch between
 * different torque prediction methods.
 *
 * @author IC_CAN Team
 * @date 2025-10-15
 */

#include "ic_can/core/torque_predictor_unified.h"
#include <iostream>
#include <iomanip>

namespace ic_can {

TorquePredictorUnified::TorquePredictorUnified() : current_method_(TorquePredictionMethod::PURE_C_MATLAB), initialized_(false) {
    std::cout << "🔧 Initializing Unified Torque Prediction System..." << std::endl;

    // Initialize Pure C MATLAB predictor
    try {
        pure_c_predictor_ = std::make_unique<TorquePredictorPureC>();
        if (pure_c_predictor_->is_initialized()) {
            std::cout << "✅ Pure C MATLAB predictor initialized" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "⚠️  Pure C MATLAB predictor failed: " << e.what() << std::endl;
        pure_c_predictor_.reset();
    }

    // Initialize Regressor-based predictor
    try {
        regressor_predictor_ = std::make_unique<TorquePredictorRegressor>();
        if (regressor_predictor_->is_initialized()) {
            std::cout << "✅ Regressor-based predictor initialized" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "⚠️  Regressor-based predictor failed: " << e.what() << std::endl;
        regressor_predictor_.reset();
    }

    // Check if any method is available
    if (pure_c_predictor_ && pure_c_predictor_->is_initialized()) {
        initialized_ = true;
        current_method_ = TorquePredictionMethod::PURE_C_MATLAB;
        std::cout << "✅ Using Pure C MATLAB torque prediction method" << std::endl;
    } else if (regressor_predictor_ && regressor_predictor_->is_initialized()) {
        initialized_ = true;
        current_method_ = TorquePredictionMethod::REGRESSOR_BASED;
        std::cout << "✅ Using Regressor-based torque prediction method" << std::endl;
    } else {
        std::cout << "❌ No torque prediction methods available!" << std::endl;
    }
}

bool TorquePredictorUnified::is_initialized() const {
    return initialized_;
}

bool TorquePredictorUnified::switch_method(TorquePredictionMethod method) {
    if (method == TorquePredictionMethod::PURE_C_MATLAB) {
        if (pure_c_predictor_ && pure_c_predictor_->is_initialized()) {
            current_method_ = method;
            std::cout << "🔧 Switched to torque prediction method: " << get_torque_method_name(method) << std::endl;
            return true;
        } else {
            std::cout << "❌ Pure C MATLAB method is not available" << std::endl;
            return false;
        }
    } else if (method == TorquePredictionMethod::REGRESSOR_BASED) {
        if (regressor_predictor_ && regressor_predictor_->is_initialized()) {
            current_method_ = method;
            std::cout << "🔧 Switched to torque prediction method: " << get_torque_method_name(method) << std::endl;
            return true;
        } else {
            std::cout << "❌ Regressor-based method is not available" << std::endl;
            return false;
        }
    } else {
        std::cout << "❌ Invalid method ID" << std::endl;
        return false;
    }
}

bool TorquePredictorUnified::predict_torques(const double q[6], const double dq[6], const double ddq[6],
                                            double M_torque[6], double C_torque[6], double G_torque[6],
                                            double total_torque[6]) {
    if (!initialized_) {
        std::cout << "❌ TorquePredictorUnified not initialized" << std::endl;
        return false;
    }

    if (current_method_ == TorquePredictionMethod::PURE_C_MATLAB) {
        if (pure_c_predictor_ && pure_c_predictor_->is_initialized()) {
            return pure_c_predictor_->predict_torques(q, dq, ddq, M_torque, C_torque, G_torque, total_torque);
        }
    } else if (current_method_ == TorquePredictionMethod::REGRESSOR_BASED) {
        if (regressor_predictor_ && regressor_predictor_->is_initialized()) {
            double F_torque[6]; // Friction torque for regressor method
            bool result = regressor_predictor_->predict_torques(q, dq, ddq, M_torque, C_torque, G_torque, F_torque, total_torque);
            // Note: F_torque is computed but not exposed in unified interface for consistency
            return result;
        }
    }

    std::cout << "❌ Current torque prediction method is not available" << std::endl;
    return false;
}

bool TorquePredictorUnified::predict_total_torque(const double q[6], const double dq[6], const double ddq[6],
                                                 double total_torque[6]) {
    if (!initialized_) {
        std::cout << "❌ TorquePredictorUnified not initialized" << std::endl;
        return false;
    }

    if (current_method_ == TorquePredictionMethod::PURE_C_MATLAB) {
        if (pure_c_predictor_ && pure_c_predictor_->is_initialized()) {
            return pure_c_predictor_->predict_total_torque(q, dq, ddq, total_torque);
        }
    } else if (current_method_ == TorquePredictionMethod::REGRESSOR_BASED) {
        if (regressor_predictor_ && regressor_predictor_->is_initialized()) {
            return regressor_predictor_->predict_total_torque(q, dq, ddq, total_torque);
        }
    }

    std::cout << "❌ Torque prediction method is not available" << std::endl;
    return false;
}

bool TorquePredictorUnified::predict_gravity_torque(const double q[6], double gravity_torque[6]) {
    if (!initialized_) {
        std::cout << "❌ TorquePredictorUnified not initialized" << std::endl;
        return false;
    }

    if (current_method_ == TorquePredictionMethod::PURE_C_MATLAB) {
        if (pure_c_predictor_ && pure_c_predictor_->is_initialized()) {
            return pure_c_predictor_->predict_gravity_torque(q, gravity_torque);
        }
    } else if (current_method_ == TorquePredictionMethod::REGRESSOR_BASED) {
        if (regressor_predictor_ && regressor_predictor_->is_initialized()) {
            return regressor_predictor_->predict_gravity_torque(q, gravity_torque);
        }
    }

    std::cout << "❌ Torque prediction method is not available" << std::endl;
    return false;
}

void TorquePredictorUnified::print_torque_breakdown(const double q[6], const double dq[6], const double ddq[6]) {
    if (!initialized_) {
        std::cout << "❌ TorquePredictorUnified not initialized" << std::endl;
        return;
    }

    if (current_method_ == TorquePredictionMethod::PURE_C_MATLAB) {
        if (pure_c_predictor_ && pure_c_predictor_->is_initialized()) {
            pure_c_predictor_->print_torque_breakdown(q, dq, ddq);
            return;
        }
    } else if (current_method_ == TorquePredictionMethod::REGRESSOR_BASED) {
        if (regressor_predictor_ && regressor_predictor_->is_initialized()) {
            regressor_predictor_->print_torque_breakdown(q, dq, ddq);
            return;
        }
    }

    std::cout << "❌ Torque prediction method is not available" << std::endl;
}

void TorquePredictorUnified::print_method_status() {
    std::cout << "\n🔧 Torque Prediction Methods Status:" << std::endl;
    std::cout << std::string(50, '=') << std::endl;

    std::cout << "Pure C MATLAB (M,C,G matrices): ";
    if (pure_c_predictor_ && pure_c_predictor_->is_initialized()) {
        std::cout << "✅ AVAILABLE" << std::endl;
    } else {
        std::cout << "❌ NOT AVAILABLE" << std::endl;
    }

    std::cout << "Regressor-Based (with friction): ";
    if (regressor_predictor_ && regressor_predictor_->is_initialized()) {
        std::cout << "✅ AVAILABLE" << std::endl;
    } else {
        std::cout << "❌ NOT AVAILABLE" << std::endl;
    }

    std::cout << "Current method: " << get_torque_method_name(current_method_) << std::endl;
    std::cout << std::string(50, '=') << std::endl;
}

std::string TorquePredictorUnified::get_method_name() const {
    return std::string(get_torque_method_name(current_method_));
}

} // namespace ic_can