/**
 * @file friction_compensated_controller.cpp
 * @brief Real-time Friction Compensated Controller
 *
 * Simple controller that applies friction compensation
 * based on pre-identified parameters.
 *
 * @author IC_CAN Team
 * @date 2025-10-27
 */

#include <iostream>
#include <iomanip>
#include <chrono>
#include <thread>
#include <fstream>
#include "../include/ic_can/core/torque_predictor_unified.h"

using namespace ic_can;

struct FrictionParams {
    double Fc;           // Coulomb friction force (N⋅m)
    double Fv;           // Viscous friction coefficient (Nm⋅s/rad)
    double velocity_threshold; // Velocity threshold for model switching
    bool enabled;       // Enable/disable compensation
};

struct MotorState {
    std::array<double, 6> q = {0, 0, 0, 0, 0, 0};      // Current positions
    std::array<double, 6> dq = {0, 0, 0, 0, 0, 0};     // Current velocities
    std::array<double, 6> tau_friction = {0, 0, 0, 0, 0, 0}; // Friction torques
    std::array<double, 6> tau_total = {0, 0, 0, 0, 0, 0};    // Total command torques
};

// Simple Stribeck friction model for smooth low-velocity transition
double stribeck_velocity(double dq, double Fc, double Fs, double vs) {
    if (std::abs(dq) < 0.001) {
        return 0.0; // Below threshold - use Coulomb only
    }

    double v_star = vs; // Stribeck velocity
    double tau_s = Fs - Fc;
    double ratio = std::abs(dq) / v_star;

    if (ratio > 1.0) ratio = 1.0; // Limit ratio

    double exp_term = std::exp(-ratio * v_star / vs);
    double tau_stribeck = tau_s + (Fs - Fc) * (1.0 - exp_term);

    return tau_stribeck;
}

double smooth_sign(double dq, double threshold = 0.01) {
    if (std::abs(dq) < threshold) {
        return dq; // Linear near zero
    } else {
        return (dq > 0) ? 1.0 : -1.0; // Smoothed sign
    }
}

int main() {
    std::cout << "=== Friction Compensated Controller ===" << std::endl;
    std::cout << "Real-time controller with friction compensation" << std::endl;

    // Initialize friction parameters (example values - these would come from your analysis)
    std::array<FrictionParams, 6> friction_params = {{
        {Fc: 1.2, Fv: 0.8, velocity_threshold: 0.1, enabled: true},  // Joint 0
        {Fc: 0.9, Fv: 0.6, velocity_threshold: 0.15, enabled: true},  // Joint 1
        {Fc: 0.7, Fv: 0.5, velocity_threshold: 0.2, enabled: true},   // Joint 2
        {Fc: 0.6, Fv: 0.4, velocity_threshold: 0.25, enabled: true},  // Joint 3
        {Fc: 0.5, Fv: 0.3, velocity_threshold: 0.3, enabled: true},  // Joint 4
        {Fc: 0.4, Fv: 0.2, velocity_threshold: 0.4, enabled: true},  // Joint 5
        {0, 0, 0, 0, 0, 0, false}  // Joint 6 (no friction)
    }};

    MotorState state;
    std::array<double, 6> q_target = {0, 0, 0, 0, 0, 0};  // Target positions
    double Kd = {0.1, 0.05, 0.1, 0.05, 0.1, 0.05}; // Small damping for compliant feel

    std::cout << "\n🔧 Controller Configuration:" << std::endl;
    for (int i = 0; i < 6; i++) {
        std::cout << "  Joint " << i << ": Fc=" << std::setw(5) << friction_params[i].Fc
                 << " Fv=" << std::setw(6) << friction_params[i].Fv
                 << " enabled=" << friction_params[i].enabled << std::endl;
    }

    std::cout << "  Velocity damping Kd: [" << std::setw(6) << Kd[0];
    for (int i = 0; i < 6; i++) {
        std::cout << std::fixed << std::setprecision(3) << Kd[i] << " ";
    }
    std::cout << "]" << std::endl;

    // Initialize torque predictor for gravity compensation
    TorquePredictorUnified predictor;
    if (!predictor.is_initialized()) {
        std::cout << "❌ Torque predictor initialization failed!" << std::endl;
        return -1;
    }

    predictor.set_method(TorquePredictionMethod::PURE_C);
    predictor.enable_gravity_compensation(true);

    std::cout << "\n🔄 Controller Loop (500Hz simulation):" << std::endl;
    std::cout << std::string(80, '=') << std::endl;

    const double dt = 0.002;  // 500Hz = 2ms
    const int num_cycles = 1000;  // 2 seconds of data
    std::ofstream data_file("controller_output.csv");

    data_file << "cycle,time_ms,q0,q1,q2,q3,q4,q5,dq0,dq1,dq2,dq3,dq4,dq5,"
                << "tau_f0,tau_f1,tau_f2,tau_f3,tau_f4,tau_f5,tau_total0,tau_total1,tau_total2,tau_total3,tau_total4,tau_total5"
                << ",model_type" << std::endl;

    // Main control loop
    for (int cycle = 0; cycle < num_cycles; ++cycle) {
        auto start_time = std::chrono::high_resolution_clock::now();

        // 1. Position control (simple move toward target)
        for (int i = 0; i < 6; i++) {
            double error = q_target[i] - state.q[i];
            state.dq[i] = 0.1 * std::tanh(error / 0.5); // Smoothed approach
            state.q[i] += state.dq[i] * dt;
        }

        // 2. Calculate friction compensation for each joint
        for (int i = 0; i < 6; i++) {
            if (!friction_params[i].enabled) {
                state.tau_friction[i] = 0.0;
                continue;
            }

            double tau_coulomb = friction_params[i].Fc * std::copysign(state.dq[i]);

            // Apply smooth transition near zero velocity
            double dq_smooth = smooth_sign(state.dq[i], friction_params[i].velocity_threshold);
            double tanh_term = std::tanh(state.dq[i] / friction_params[i].velocity_threshold);

            // Coulomb + Viscous + Stribeck model
            state.tau_friction[i] = tau_coulomb +
                                   friction_params[i].Fv * state.dq[i] * tanh_term +
                                   stribeck_velocity(state.dq[i], friction_params[i].Fc, 2.0, 1.0);

            // Simple sign-based selection (you can modify this logic)
            if (std::abs(state.dq[i]) < friction_params[i].velocity_threshold) {
                // Use basic Coulomb + Viscous at low velocities
            } else {
                // Keep Stribeck model at higher velocities
            }
        }

        // 3. Get gravity compensation (would be g(q) from IC_CAN system)
        std::array<double, 6> tau_gravity = {0, 0, 0, 0, 0, 0};
        predictor.predict_gravity_torque(state.q.data(), tau_gravity.data());

        // 4. Apply damping to joint velocities for smoother motion
        for (int i = 0; i < 6; i++) {
            state.dq[i] = (1.0 - Kd[i]) * state.dq[i];
        }

        // 5. Calculate total command torques
        for (int i = 0; i < 6; i++) {
            state.tau_total[i] = tau_gravity[i] + state.tau_friction[i];
        }

        auto end_time = std::chrono::high_resolution_clock::now();
        double elapsed_ms = std::chrono::duration<double, std::milli>(end_time - start_time).count();

        // 6. Log data to CSV
        data_file << cycle << "," << elapsed_ms;
        for (int i = 0; i < 6; i++) {
            data_file << "," << state.q[i];
        }
        for (int i = 0; i < 6; i++) {
            data_file << "," << state.dq[i];
        }
        for (int i = 0; i < 6; i++) {
            data_file << "," << state.tau_friction[i];
        }
        for (int i = 0; i < 6; i++) {
            data_file << "," << tau_gravity[i];
        }
        for (int i = 0; i < 6; i++) {
            data_file << "," << state.tau_total[i];
        }

        // Determine active friction model
        std::string model_type = "basic";
        for (int i = 0; i < 6; i++) {
            if (std::abs(state.dq[i]) > friction_params[i].velocity_threshold) {
                model_type = "stribeck";
                break;
            }
        }

        data_file << "," << model_type << std::endl;

        // 7. Output status
        if (cycle % 100 == 0) {
            std::cout << "\r📊 Cycle " << cycle << ": q=[";
            for (int i = 0; i < 6; i++) {
                std::cout << std::fixed << std::setprecision(2) << state.q[i];
                if (i < 5) std::cout << ",";
            }
            std::cout << "], τ_f=[" << std::fixed << std::setprecision(3);
            for (int i = 0; i < 6; i++) {
                std::cout << std::fixed << std::setprecision(3) << state.tau_friction[i];
                if (i < 5) std::cout << ",";
            }
            std::cout << "] Nm, model=" << model_type << std::endl;
        }

        // Small delay to simulate real-time 500Hz
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }

    data_file.close();
    std::cout << "\n✅ Controller simulation completed!" << std::endl;
    std::cout << "📊 Data saved to: controller_output.csv" << std::endl;
    std::cout << "🎯 Ready for integration with IC_CAN motor control!" << std::endl;

    return 0;
}