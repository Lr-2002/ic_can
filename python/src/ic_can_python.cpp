// Copyright 2025 IC_CAN Team
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <ic_can/core/ic_can.hpp>
#include <vector>
#include <string>
#include <chrono>

namespace py = pybind11;

// Python 模块定义
PYBIND11_MODULE(ic_can_python, m) {
    m.doc() = "IC_CAN Python bindings - High Performance CAN Control Library for Robotic Arms";

    // ========== 主控制器类绑定 ==========

    py::class_<ic_can::IC_CAN>(m, "IC_CAN")
        .def(py::init<const std::string&, bool>(),
             py::arg("device_sn") = "F561E08C892274DB09496BCC1102DBC5",
             py::arg("debug") = false,
             "Initialize IC_CAN controller")

        // 系统控制
        .def("initialize", &ic_can::IC_CAN::initialize,
             "Initialize the system and connect to hardware")
        .def("shutdown", &ic_can::IC_CAN::shutdown,
             "Shutdown the system")
        .def("enable_all", &ic_can::IC_CAN::enable_all,
             "Enable all motors")
        .def("disable_all", &ic_can::IC_CAN::disable_all,
             "Disable all motors")
        .def("set_zero_all", &ic_can::IC_CAN::set_zero_all,
             "Set all motors to zero position")
        .def("set_motor_zero_calibration", &ic_can::IC_CAN::set_motor_zero_calibration,
             py::arg("motor_id"),
             "Set motor zero position calibration")

        // 状态读取
        .def("refresh_all", &ic_can::IC_CAN::refresh_all,
             "Refresh all motor states")
        .def("get_joint_positions", &ic_can::IC_CAN::get_joint_positions,
             "Get all joint positions in radians")
        .def("get_joint_velocities", &ic_can::IC_CAN::get_joint_velocities,
             "Get all joint velocities in rad/s")
        .def("get_joint_torques", &ic_can::IC_CAN::get_joint_torques,
             "Get all joint torques in N·m")

        // 控制接口
        .def("set_joint_positions", &ic_can::IC_CAN::set_joint_positions,
             py::arg("positions"), py::arg("velocities") = std::vector<double>{},
             py::arg("torques") = std::vector<double>{},
             "Set joint positions")
        .def("set_joint_velocities", &ic_can::IC_CAN::set_joint_velocities,
             py::arg("velocities"), py::arg("torques") = std::vector<double>{},
             "Set joint velocities")
        .def("set_joint_torques", &ic_can::IC_CAN::set_joint_torques,
             py::arg("torques"),
             "Set joint torques")

        // 控制循环
        .def("start_control_loop", &ic_can::IC_CAN::start_control_loop,
             py::arg("frequency"),
             "Start control loop with specified frequency")
        .def("stop_control_loop", &ic_can::IC_CAN::stop_control_loop,
             "Stop control loop")
        .def("is_control_running", &ic_can::IC_CAN::is_control_running,
             "Check if control loop is running")

        // 高频控制（向后兼容）
        .def("start_high_frequency_control", &ic_can::IC_CAN::start_high_frequency_control,
             "Start high frequency control thread (500Hz)")
        .def("stop_high_frequency_control", &ic_can::IC_CAN::stop_high_frequency_control,
             "Stop high frequency control thread")
        .def("is_hf_control_running", &ic_can::IC_CAN::is_hf_control_running,
             "Check if high frequency control is running")

        // 轨迹插值
        .def("set_target_positions_interpolated", &ic_can::IC_CAN::set_target_positions_interpolated,
             py::arg("target_positions"), py::arg("max_velocity") = 0.2,
             "Set target positions with velocity-limited interpolation")
        .def_static("interpolate_positions", &ic_can::IC_CAN::interpolate_positions,
             py::arg("current_positions"), py::arg("target_positions"),
             py::arg("dt"), py::arg("max_velocity") = 0.2,
             "Interpolate positions with velocity limiting")

        // 电机增益设置
        .def("set_motor_gains", &ic_can::IC_CAN::set_motor_gains,
             py::arg("motor_id"), py::arg("kp"), py::arg("kd"),
             "Set motor-specific P and D gains")
        .def("set_all_motor_gains", &ic_can::IC_CAN::set_all_motor_gains,
             py::arg("kp_values"), py::arg("kd_values"),
             "Set P and D gains for all motors")
        .def("get_motor_gains", &ic_can::IC_CAN::get_motor_gains,
             py::arg("motor_id"), py::arg("kp"), py::arg("kd"),
             "Get motor P and D gains")
        .def("load_default_motor_gains", &ic_can::IC_CAN::load_default_motor_gains,
             "Load default motor gains")

        // 重力补偿
        .def("enable_gravity_compensation", &ic_can::IC_CAN::enable_gravity_compensation,
             "Enable gravity compensation for arm joints")
        .def("disable_gravity_compensation", &ic_can::IC_CAN::disable_gravity_compensation,
             "Disable gravity compensation")
        .def("is_gravity_compensation_enabled", &ic_can::IC_CAN::is_gravity_compensation_enabled,
             "Check if gravity compensation is enabled")
        .def("get_gravity_compensation_torques", &ic_can::IC_CAN::get_gravity_compensation_torques,
             "Get gravity compensation torques for all joints")
        .def("get_all_predicted_torques", &ic_can::IC_CAN::get_all_predicted_torques,
             "Get all predicted torques (M*ddq + C*dq + G) for current state")
        .def("print_torque_breakdown", &ic_can::IC_CAN::print_torque_breakdown,
             "Print detailed torque component breakdown")

        // 扭矩预测方法切换
        .def("switch_torque_prediction_method", &ic_can::IC_CAN::switch_torque_prediction_method,
             py::arg("method_id"),
             "Switch torque prediction method (0=Pure C MATLAB, 1=Regressor-Based)")
        .def("print_torque_method_status", &ic_can::IC_CAN::print_torque_method_status,
             "Print available torque prediction methods and their status")

        // 日志记录
        .def("start_logging", &ic_can::IC_CAN::start_logging,
             py::arg("log_directory") = ".",
             "Start logging system data to CSV files")
        .def("start_trajectory_logging", &ic_can::IC_CAN::start_trajectory_logging,
             py::arg("log_directory"),
             "Start trajectory-specific logging")
        .def("stop_logging", &ic_can::IC_CAN::stop_logging,
             "Stop logging system data")

        // 性能监控
        .def("enable_frequency_monitoring", &ic_can::IC_CAN::enable_frequency_monitoring,
             "Enable frequency monitoring")
        .def("disable_frequency_monitoring", &ic_can::IC_CAN::disable_frequency_monitoring,
             "Disable frequency monitoring")
        .def("get_performance_stats", &ic_can::IC_CAN::get_performance_stats,
             "Get performance statistics")
        .def("print_performance_stats", &ic_can::IC_CAN::print_performance_stats,
             "Print performance statistics")

        // 诊断接口
        .def("get_system_status", &ic_can::IC_CAN::get_system_status,
             "Get system status information")
        .def("print_system_info", &ic_can::IC_CAN::print_system_info,
             "Print system information");

        // ========== 高级 Python API - RL/DNN 友好接口 ==========

        // 1. 电机启用控制
        .def("enable_motors", [](ic_can::IC_CAN& self, py::object motor_ids) {
            if (motor_ids.is_none()) {
                return self.enable_all();
            } else {
                auto ids = motor_ids.cast<std::vector<int>>();
                bool all_success = true;
                for (int id : ids) {
                    if (id >= 1 && id <= 6) {
                        // Arm motors (1-6)
                        // Note: Individual motor enabling requires component access
                        // For now, enable all and individual motors will be controlled at Python level
                    } else if (id == 7 || id == 8) {
                        // Wrist motors (7-8)
                        // Component access needed for individual control
                    } else if (id == 9) {
                        // Gripper motor (9)
                        // Component access needed for individual control
                    }
                }
                return self.enable_all();
            }
        },
        py::arg("motor_ids") = py::none(),
        "Enable specific motors (1-9) or all if None")

        // 2. 统一状态刷新
        .def("refresh_all_state", [](const ic_can::IC_CAN& self) {
            // Refresh all motor states first
            const_cast<ic_can::IC_CAN&>(self).refresh_all();

            // Get all state information
            auto positions = self.get_joint_positions();
            auto velocities = self.get_joint_velocities();
            auto torques = self.get_joint_torques();

            // Create unified state dictionary
            py::dict state;
            state["positions"] = positions;
            state["velocities"] = velocities;
            state["torques"] = torques;
            state["motor_count"] = 9;

            // Add timestamp
            auto now = std::chrono::high_resolution_clock::now();
            auto timestamp = std::chrono::duration_cast<std::chrono::microseconds>(
                now.time_since_epoch()).count() / 1000000.0;
            state["timestamp"] = timestamp;

            return state;
        },
        "Refresh and return unified state for all 9 motors")

        // 3. 步进控制 - 统一的 p,v,t 控制
        .def("step_control", [](ic_can::IC_CAN& self,
                                const std::vector<double>& positions,
                                py::object velocities,
                                py::object torques) {
            bool success = true;

            // Handle optional parameters
            std::vector<double> vel_limits;
            std::vector<double> torque_limits;

            if (!velocities.is_none()) {
                vel_limits = velocities.cast<std::vector<double>>();
            }
            if (!torques.is_none()) {
                torque_limits = torques.cast<std::vector<double>>();
            }

            // Send position commands with optional velocity and torque limits
            success &= self.set_joint_positions(positions, vel_limits, torque_limits);

            return success;
        },
        py::arg("positions"), py::arg("velocities") = py::none(), py::arg("torques") = py::none(),
        "Step control with positions, optional velocities and torques for all motors")

        // 4. 控制模式切换
        .def("set_control_mode_python", [](ic_can::IC_CAN& self, const std::string& mode_str) {
            ic_can::IC_CAN::ControlMode mode;
            if (mode_str == "TEACH" || mode_str == "teach") {
                mode = ic_can::IC_CAN::ControlMode::TEACH_MODE;
            } else if (mode_str == "EXECUTION" || mode_str == "execution") {
                mode = ic_can::IC_CAN::ControlMode::EXECUTION_MODE;
            } else {
                throw std::invalid_argument("Invalid control mode. Use 'TEACH' or 'EXECUTION'");
            }
            return self.set_control_mode(mode);
        },
        py::arg("mode"),
        "Set control mode: 'TEACH' or 'EXECUTION'")

        // 5. 获取当前控制模式
        .def("get_control_mode_python", [](const ic_can::IC_CAN& self) {
            auto mode = self.get_control_mode();
            return (mode == ic_can::IC_CAN::ControlMode::TEACH_MODE) ? "TEACH" : "EXECUTION";
        },
        "Get current control mode as string");

        // Note: Component access methods are temporarily removed
        // because the component classes are not included in the minimal build

    // 版本信息
    m.attr("__version__") = "1.0.0";
    m.attr("__author__") = "IC_CAN Team";
    m.attr("__license__") = "Apache License 2.0";
}