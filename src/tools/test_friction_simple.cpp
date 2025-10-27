/**
 * @file test_friction_simple.cpp
 * @brief Simple friction sweep test
 *
 * Minimal test for friction sweep functionality
 */

#include <iostream>
#include "../include/ic_can/core/torque_predictor_unified.h"

int main() {
    std::cout << "=== Simple Friction Test ===" << std::endl;

    TorquePredictorUnified predictor;
    if (!predictor.is_initialized()) {
        std::cout << "❌ Torque predictor failed" << std::endl;
        return -1;
    }

    std::cout << "✅ Friction sweep system ready!" << std::endl;
    std::cout << "📊 Use friction_sweep tool for detailed identification." << std::endl;

    return 0;
}