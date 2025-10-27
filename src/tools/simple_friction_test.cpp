/**
 * @file simple_friction_test.cpp
 * @brief Minimal Friction Test
 *
 * Simple test to validate friction compensation
 * without requiring torque predictor dependencies
 *
 * @author IC_CAN Team
 * @date 2025-10-27
 */

#include <iostream>
#include <iomanip>
#include <vector>
#include <array>

int main() {
    std::cout << "=== Simple Friction Test ===" << std::endl;
    std::cout << "Testing friction compensation without heavy dependencies" << std::endl;

    // Simple friction model parameters (example values)
    std::array<double, 6> friction_params = {
        {Fc: 0.5, Fv: 0.1, enabled: true},  // Joint 0
        {Fc: 0.3, Fv: 0.08, enabled: true},  // Joint 1
        {Fc: 0.2, Fv: 0.15, enabled: true},  // Joint 2
        {0.0, 0.0, 0.0, 0.0, false}  // Joint 3 (disabled)
        {0.0, 0.0, 0.0, 0.0, false},  // Joint 4 (disabled)
        {0.0, 0.0, 0.0, 0.0, false},  // Joint 5 (disabled)
    };

    std::cout << "\n📊 Friction Parameters:" << std::endl;
    for (int i = 0; i < 6; ++i) {
        std::cout << "Joint " << i << ": Fc=" << std::setw(5) << friction_params[i].Fc
                 << " Fv=" << std::setw(6) << friction_params[i].Fv
                 << " enabled=" << friction_params[i].enabled << std::endl;
    }

    std::cout << "\n🔄 Testing friction compensation:" << std::endl;

    // Test basic friction compensation
    for (int test = 0; test < 3; ++test) {
        std::cout << "\n--- Test " << (test + 1) << ": High velocity ---" << std::endl;

        double q = 0.5;
        double dq = 2.0;
        double ddq = 0.0;

        // Coulomb friction (opposes motion)
        double tau_coulomb = friction_params[0].Fc * std::copysign(dq);

        // Viscous friction (opposes velocity)
        double tau_viscous = friction_params[0].Fv * dq;

        // Total friction torque
        double tau_friction = tau_coulomb + tau_viscous;

        std::cout << "  q̙ = " << q << " rad" << std::endl;
        std::cout << "  dq̙ = " << dq << " rad/s" << std::endl;
        std::cout << "  Fc = " << friction_params[0].Fc << " N⋅m" << std::endl;
        std::cout << "  Fv = " << friction_params[0].Fv << " Nm⋅s/rad" << std::endl;
        std::cout << "  τ_friction = " << tau_friction << " N⋅m" << std::endl;
        std::cout << "  τ_total = " << tau_friction << " N⋅m" << std::endl;
        std::cout << "  (Coulomb = " << tau_coulomb << " N⋅m, Viscous = " << tau_viscous << " N⋅m)" << std::endl;
    }

    return 0;
}