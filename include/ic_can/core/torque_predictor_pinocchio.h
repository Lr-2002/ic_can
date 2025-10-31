/**
 * @file torque_predictor_pinocchio.h
 * @brief Pinocchio-based Torque Prediction System
 *
 * Pinocchio-based torque predictor that reads URDF and calculates
 * precise gravity and Coriolis torque components.
 *
 * @author IC_CAN Team
 * @date 2025-10-27
 */

#ifndef IC_CAN_TORQUE_PREDICTOR_PINOCCHIO_H
#define IC_CAN_TORQUE_PREDICTOR_PINOCCHIO_H

#include <pinocchio/parsers/urdf.hpp>
#include <pinocchio/algorithm/rnea.hpp>
#include <pinocchio/algorithm/crba.hpp>
#include <pinocchio/algorithm/kinematics.hpp>
#include <pinocchio/algorithm/jacobian.hpp>
#include <pinocchio/algorithm/joint-configuration.hpp>
#include <pinocchio/spatial/explog.hpp>
#include <Eigen/Dense>
#include <string>
#include <vector>

namespace ic_can {

/**
 * @brief Pinocchio-based Torque Predictor
 *
 * This class uses Pinocchio library to read URDF and compute
 * accurate torque components including gravity (G) and Coriolis (C)
 * torques for the 6-DOF IC arm.
 */
class TorquePredictorPinocchio {
private:
    // Pinocchio model and data
    pinocchio::Model model_;
    pinocchio::Data data_;

    // Robot configuration
    std::string urdf_path_;
    bool initialized_;

    // Joint configuration (variable DOF: 6-arm, 8-arm+wrist, 9-arm+wrist+gripper)
    int dof_;                        // Current degrees of freedom
    std::vector<std::string> joint_names_;

    // Gravity vector (default: [0, 0, -9.81])
    Eigen::Vector3d gravity_;

    // Cache for computed torque components
    Eigen::VectorXd q_config_;      // Joint configuration vector
    Eigen::VectorXd v_config_;      // Joint velocity vector
    Eigen::VectorXd a_config_;      // Joint acceleration vector

    /**
     * @brief Initialize joint names and DOF based on URDF
     */
    void initialize_joint_configuration();

    /**
     * @brief Validate joint configuration
     * @param q Joint positions
     * @param dq Joint velocities
     * @param ddq Joint accelerations
     * @return true if valid
     */
    bool validate_joint_config(const double* q, const double* dq, const double* ddq);

public:
    /**
     * @brief Constructor
     * @param urdf_path Path to the URDF file (optional, uses default if empty)
     */
    explicit TorquePredictorPinocchio(const std::string& urdf_path = "");

    /**
     * @brief Destructor
     */
    ~TorquePredictorPinocchio() = default;

    /**
     * @brief Initialize the predictor with URDF
     * @param urdf_path Path to URDF file (optional, uses default if empty)
     * @return true if initialization successful
     */
    bool initialize(const std::string& urdf_path = "");

    /**
     * @brief Check if predictor is initialized
     * @return true if initialized and ready for predictions
     */
    bool is_initialized() const { return initialized_; }

    /**
     * @brief Set gravity vector
     * @param gravity 3D gravity vector [gx, gy, gz]
     */
    void set_gravity(const Eigen::Vector3d& gravity);

    /**
     * @brief Set gravity vector from double array
     * @param gravity 3D gravity vector [gx, gy, gz]
     */
    void set_gravity(const double gravity[3]);

    /**
     * @brief Get current gravity vector
     * @return Current gravity vector
     */
    Eigen::Vector3d get_gravity() const { return gravity_; }

    /**
     * @brief Get the URDF path used
     * @return URDF file path
     */
    const std::string& get_urdf_path() const { return urdf_path_; }

    /**
     * @brief Predict all torque components (M, C, G)
     * @param q Joint positions [dof_]
     * @param dq Joint velocities [dof_]
     * @param ddq Joint accelerations [dof_]
     * @param M_torque Output mass/inertia torque component [dof_]
     * @param C_torque Output Coriolis torque component [dof_]
     * @param G_torque Output gravity torque component [dof_]
     * @param total_torque Output total predicted torque [dof_]
     * @return true if prediction successful
     */
    bool predict_torques(const double* q, const double* dq, const double* ddq,
                        double* M_torque, double* C_torque, double* G_torque,
                        double* total_torque);

    /**
     * @brief Predict total torque only
     * @param q Joint positions [dof_]
     * @param dq Joint velocities [dof_]
     * @param ddq Joint accelerations [dof_]
     * @param total_torque Output total predicted torque [dof_]
     * @return true if prediction successful
     */
    bool predict_total_torque(const double* q, const double* dq, const double* ddq,
                             double* total_torque);

    /**
     * @brief Predict gravity torques only
     * @param q Joint positions [dof_]
     * @param gravity_torque Output gravity torques [dof_]
     * @return true if prediction successful
     */
    bool predict_gravity_torque(const double* q, double* gravity_torque);

    /**
     * @brief Compute mass matrix
     * @param q Joint positions [dof_]
     * @param mass_matrix Output dof_x_dof_ mass matrix
     * @return true if computation successful
     */
    bool compute_mass_matrix(const double* q, Eigen::MatrixXd& mass_matrix);

    /**
     * @brief Compute Coriolis matrix
     * @param q Joint positions [dof_]
     * @param dq Joint velocities [dof_]
     * @param coriolis_matrix Output dof_x_dof_ Coriolis matrix
     * @return true if computation successful
     */
    bool compute_coriolis_matrix(const double* q, const double* dq,
                                Eigen::MatrixXd& coriolis_matrix);

    /**
     * @brief Print detailed torque breakdown for analysis
     * @param q Joint positions [dof_]
     * @param dq Joint velocities [dof_]
     * @param ddq Joint accelerations [dof_]
     */
    void print_torque_breakdown(const double* q, const double* dq, const double* ddq);

    /**
     * @brief Print robot model information
     */
    void print_model_info();

    /**
     * @brief Get number of degrees of freedom
     * @return DOF count
     */
    int get_dof() const { return dof_; }

    /**
     * @brief Get joint names
     * @return Vector of joint names
     */
    const std::vector<std::string>& get_joint_names() const { return joint_names_; }

    /**
     * @brief Validate if a configuration is within joint limits
     * @param q Joint positions [dof_]
     * @return true if within limits
     */
    bool is_within_joint_limits(const double* q);

    /**
     * @brief Get joint limits
     * @param lower Output lower limits [dof_]
     * @param upper Output upper limits [dof_]
     * @return true if limits are available
     */
    bool get_joint_limits(double* lower, double* upper);
};

} // namespace ic_can

#endif // IC_CAN_TORQUE_PREDICTOR_PINOCCHIO_H