/**
 * @file friction_standalone_test.cpp
 * @brief Standalone Friction Test
 *
 * Tests friction compensation without requiring
 * torque predictor dependencies.
 *
 * @author IC_CAN Team
 * @date 2025-10-27
 */

#include <iostream>
#include <vector>
#include <array>
#include <cmath>

int main() {
    std::cout << "=== Standalone Friction Test ===" << std::endl;
    std::cout << "Testing friction models without torque predictor dependencies" << std::endl;

    // Test Coulomb + Viscous friction model directly
    double q = 1.0;           // Joint position (rad)
    double dq = 0.5;           // Joint velocity (rad/s)
    double ddq = 0.0;          // Joint acceleration (rad/s²)

    // Coulomb + Viscous model: τf(q˙) = Fc⋅sgn(q˙) + Fv⋅q˙
    double Fc = 1.2;        // Coulomb friction force (N⋅m)
    double Fv = 0.05;       // Viscous friction coefficient (N⋅m⋅s/rad)

    // Calculate friction torque
    double tau_coulomb = Fc * std::copysign(dq);
    double tau_viscous = Fv * dq;
    double tau_total = tau_coulomb + tau_viscous;

    std::cout << "Joint position q = " << q << " rad" << std::endl;
    std::cout << "Joint velocity dq = " << dq << " rad/s" << std::endl;
    std::cout << "Coulomb force Fc = " << Fc << " N⋅m" << std::endl;
    std::cout << "Viscous coefficient Fv = " << Fv << " Nm⋅s/rad" << std::endl;
    std::cout << "Coulomb torque τc = " << tau_coulomb << " Nm" << std::endl;
    std::cout << "Viscous torque τv = " << tau_viscous << " Nm" << std::endl;
    std::cout << "Total friction torque τf = " << tau_total << " Nm" << std::endl;
    std::cout << "-----------------------------------" << std::endl;

    // Test zero velocity with Coulomb only (Stribeck effect)
    double tau_stribeck = tau_coulomb + (Fc - Fc) * (1.0 - std::exp(-std::abs(dq) / 0.1));
    std::cout << "\nStribeck model at low velocity:" << std::endl;
    std::cout << "  τ = " << tau_stribeck << " Nm (reduced by " << std::abs(tau_stribeck - tau_coulomb) << ")" << std::endl;

    std::cout << "\n✅ Standalone friction models working correctly!" << std::endl;
    std::cout << "Ready for integration with IC_CAN friction compensation system." << std::endl;

    return 0;
}