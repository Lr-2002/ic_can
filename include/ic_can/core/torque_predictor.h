/**
 * @file torque_predictor.h
 * @brief High-Performance Torque Prediction System for IC_CAN
 *
 * Provides physics-based torque prediction using pure C MATLAB functions
 * for real-time robotic arm control applications.
 *
 * @author IC_CAN Team
 * @date 2025-10-15
 */

#ifndef IC_CAN_TORQUE_PREDICTOR_H
#define IC_CAN_TORQUE_PREDICTOR_H

#include <array>
#include <string>

namespace ic_can {

/**
 * @brief Torque component breakdown structure
 */
struct TorqueComponents {
    std::array<double, 6> mass_torque;      ///< M(q)·ddq - Inertia torque
    std::array<double, 6> coriolis_torque;  ///< C(q,dq)·dq - Coriolis torque
    std::array<double, 6> gravity_torque;   ///< G(q) - Gravity torque
    std::array<double, 6> total_torque;     ///< Total predicted torque

    TorqueComponents() {
        mass_torque.fill(0.0);
        coriolis_torque.fill(0.0);
        gravity_torque.fill(0.0);
        total_torque.fill(0.0);
    }
};

/**
 * @brief High-performance torque predictor using pure C MATLAB functions
 *
 * This class provides physics-based torque prediction for 6-DOF robotic arms
 * using the dynamic equation: τ = M(q)·ddq + C(q,dq)·dq + G(q)
 *
 * Performance characteristics:
 * - ~294,000 Hz computation frequency
 * - ~3.4 μs average computation time
 * - Suitable for real-time control >1kHz
 */
class TorquePredictor {
private:
    static constexpr int NUM_JOINTS = 6;
    static constexpr int NUM_PARAMS = 66;
    static constexpr int MATRIX_SIZE = 36; // 6x6 flattened

    std::array<double, NUM_PARAMS> parameters_;
    bool initialized_;

    // Private helper methods
    bool load_parameters(const std::string& filename);
    void compute_total_torque(const double* M_matrix, const double* C_matrix,
                             const double* G_vector, const double* dq, const double* ddq,
                             double* total_torque);

public:
    /**
     * @brief Constructor - initializes MATLAB Coder runtime and loads parameters
     */
    TorquePredictor();

    /**
     * @brief Destructor - terminates MATLAB Coder runtime
     */
    ~TorquePredictor();

    /**
     * @brief Check if predictor is properly initialized
     * @return true if ready for torque prediction
     */
    bool is_initialized() const { return initialized_; }

    /**
     * @brief Predict complete torque components for given joint state
     * @param q Joint positions [6] (rad)
     * @param dq Joint velocities [6] (rad/s)
     * @param ddq Joint accelerations [6] (rad/s²)
     * @param torques Output torque components structure
     * @return true if prediction successful
     */
    bool predict_torques(const std::array<double, NUM_JOINTS>& q,
                        const std::array<double, NUM_JOINTS>& dq,
                        const std::array<double, NUM_JOINTS>& ddq,
                        TorqueComponents& torques);

    /**
     * @brief Predict only total torque for high-performance applications
     * @param q Joint positions [6] (rad)
     * @param dq Joint velocities [6] (rad/s)
     * @param ddq Joint accelerations [6] (rad/s²)
     * @param total_torque Output total torque [6]
     * @return true if prediction successful
     */
    bool predict_total_torque(const double q[NUM_JOINTS],
                             const double dq[NUM_JOINTS],
                             const double ddq[NUM_JOINTS],
                             double total_torque[NUM_JOINTS]);

    /**
     * @brief Get gravity torque only (useful for gravity compensation)
     * @param q Joint positions [6] (rad)
     * @param gravity_torque Output gravity torque [6]
     * @return true if prediction successful
     */
    bool predict_gravity_torque(const std::array<double, NUM_JOINTS>& q,
                               std::array<double, NUM_JOINTS>& gravity_torque);

    /**
     * @brief Print detailed torque breakdown for debugging
     * @param q Joint positions [6] (rad)
     * @param dq Joint velocities [6] (rad/s)
     * @param ddq Joint accelerations [6] (rad/s²)
     */
    void print_torque_breakdown(const std::array<double, NUM_JOINTS>& q,
                               const std::array<double, NUM_JOINTS>& dq,
                               const std::array<double, NUM_JOINTS>& ddq);

    /**
     * @brief Run performance test to verify computation speed
     * @param num_iterations Number of test iterations
     * @return achieved frequency in Hz
     */
    double run_performance_test(int num_iterations = 10000);
};

} // namespace ic_can

#endif // IC_CAN_TORQUE_PREDICTOR_H