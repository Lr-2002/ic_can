/**
 * @file torque_predictor_unified.cpp
 * @brief Unified Torque Prediction System Implementation
 *
 * Simplified implementation using only Pinocchio for reliable torque prediction.
 *
 * @author IC_CAN Team
 * @date 2025-10-15
 */

#include "ic_can/core/torque_predictor_unified.h"
#include <iostream>
#include <iomanip>

namespace ic_can {

TorquePredictorUnified::TorquePredictorUnified()
    : current_method_(TorquePredictionMethod::PINOCCHIO_URDF), initialized_(false) {

    std::cout << "🔧 Initializing Unified Torque Prediction System..." << std::endl;

    // Initialize Pinocchio predictor
    try {
        pinocchio_predictor_ = std::make_unique<TorquePredictorPinocchio>();
        if (pinocchio_predictor_->is_initialized()) {
            current_method_ = TorquePredictionMethod::PINOCCHIO_URDF;
            initialized_ = true;
            std::cout << "✅ Pinocchio torque predictor initialized successfully" << std::endl;
        } else {
            pinocchio_predictor_.reset();
            std::cout << "⚠️  Pinocchio predictor initialization failed" << std::endl;
        }
    } catch (const std::exception& e) {
        pinocchio_predictor_.reset();
        std::cout << "❌ Pinocchio predictor error: " << e.what() << std::endl;
    }

    if (initialized_) {
        std::cout << "✅ Unified torque predictor initialized with method: "
                  << get_method_name() << std::endl;
    } else {
        std::cout << "❌ Torque predictor initialization failed" << std::endl;
    }
}

bool TorquePredictorUnified::is_initialized() const {
    return initialized_;
}

bool TorquePredictorUnified::switch_method(TorquePredictionMethod method) {
    // Only support Pinocchio method
    if (method == TorquePredictionMethod::PINOCCHIO_URDF) {
        if (pinocchio_predictor_ && pinocchio_predictor_->is_initialized()) {
            current_method_ = method;
            std::cout << "🔧 Using torque prediction method: Pinocchio URDF" << std::endl;
            return true;
        }
    }

    std::cout << "❌ Failed to switch to method: "
              << "Only Pinocchio URDF method is available" << std::endl;
    return false;
}

bool TorquePredictorUnified::predict_torques(const double q[6], const double dq[6], const double ddq[6],
                                            double M_torque[6], double C_torque[6], double G_torque[6],
                                            double total_torque[6]) {
    if (!initialized_) {
        std::cout << "❌ Unified torque predictor not initialized" << std::endl;
        return false;
    }

    if (current_method_ == TorquePredictionMethod::PINOCCHIO_URDF && pinocchio_predictor_) {
        return pinocchio_predictor_->predict_torques(q, dq, ddq, M_torque, C_torque, G_torque, total_torque);
    }

    return false;
}

bool TorquePredictorUnified::predict_total_torque(const double q[6], const double dq[6], const double ddq[6],
                                                 double total_torque[6]) {
    if (!initialized_) {
        std::cout << "❌ Unified torque predictor not initialized" << std::endl;
        return false;
    }

    if (current_method_ == TorquePredictionMethod::PINOCCHIO_URDF && pinocchio_predictor_) {
        return pinocchio_predictor_->predict_total_torque(q, dq, ddq, total_torque);
    }

    return false;
}

bool TorquePredictorUnified::predict_gravity_torque(const double q[6], double gravity_torque[6]) {
    if (!initialized_) {
        std::cout << "❌ Unified torque predictor not initialized" << std::endl;
        return false;
    }

    if (current_method_ == TorquePredictionMethod::PINOCCHIO_URDF && pinocchio_predictor_) {
        return pinocchio_predictor_->predict_gravity_torque(q, gravity_torque);
    }

    return false;
}

void TorquePredictorUnified::print_torque_breakdown(const double q[6], const double dq[6], const double ddq[6]) {
    if (!initialized_) {
        std::cout << "❌ Unified torque predictor not initialized" << std::endl;
        return;
    }

    if (current_method_ == TorquePredictionMethod::PINOCCHIO_URDF && pinocchio_predictor_) {
        pinocchio_predictor_->print_torque_breakdown(q, dq, ddq);
        return;
    }

    std::cout << "❌ No valid torque predictor available for breakdown" << std::endl;
}

void TorquePredictorUnified::print_method_status() {
    std::cout << "\n🔧 Torque Prediction System Status:" << std::endl;
    std::cout << std::string(50, '=') << std::endl;

    if (pinocchio_predictor_ && pinocchio_predictor_->is_initialized()) {
        std::cout << "✅ Pinocchio URDF: Available" << std::endl;
    } else {
        std::cout << "❌ Pinocchio URDF: Not available" << std::endl;
    }

    std::cout << "Current method: " << get_method_name() << std::endl;
    std::cout << std::string(50, '=') << std::endl;
}

std::string TorquePredictorUnified::get_method_name() const {
    switch (current_method_) {
        case TorquePredictionMethod::PINOCCHIO_URDF:
            return "Pinocchio URDF (precise dynamics)";
        case TorquePredictionMethod::PURE_C_MATLAB:
            return "Pure C MATLAB (not available)";
        case TorquePredictionMethod::REGRESSOR_BASED:
            return "Regressor-based (not available)";
        default:
            return "Unknown method";
    }
}

} // namespace ic_can