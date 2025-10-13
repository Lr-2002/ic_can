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
#include <pybind11/chrono.h>
#include <pybind11/functional.h>

#include <ic_can/core/ic_can.hpp>
#include <ic_can/motors/base_motor.hpp>
#include <ic_can/motors/dm_motor.hpp>
#include <ic_can/core/arm_component.hpp>
#include <ic_can/core/gripper_component.hpp>
#include <ic_can/protocols/usb2can_adapter.hpp>
#include <ic_can/safety/safety_module.hpp>

#include <vector>
#include <string>
#include <chrono>

namespace py = pybind11;

// 类型别名
using MotorType = ic_can::MotorType;
using DMMotorType = ic_can::DMMotorType;
using DMControlMode = ic_can::DMControlMode;
using DMRegisterID = ic_can::DMRegisterID;
using SafetyViolationType = ic_can::SafetyViolationType;

// 辅助函数：转换 MotorType
std::string motor_type_to_string(MotorType type) {
    switch (type) {
        case MotorType::DM_DAMIAO: return "DM_DAMIAO";
        case MotorType::HT_HIGH_TORQUE: return "HT_HIGH_TORQUE";
        case MotorType::SERVO: return "SERVO";
        default: return "UNKNOWN";
    }
}

MotorType string_to_motor_type(const std::string& type_str) {
    if (type_str == "DM_DAMIAO") return MotorType::DM_DAMIAO;
    if (type_str == "HT_HIGH_TORQUE") return MotorType::HT_HIGH_TORQUE;
    if (type_str == "SERVO") return MotorType::SERVO;
    throw std::invalid_argument("Invalid motor type: " + type_str);
}

// 辅助函数：转换 DMMotorType
std::string dm_motor_type_to_string(DMMotorType type) {
    switch (type) {
        case DMMotorType::DM4310: return "DM4310";
        case DMMotorType::DM6248: return "DM6248";
        case DMMotorType::DM4340: return "DM4340";
        case DMMotorType::DM10010L: return "DM10010L";
        default: return "UNKNOWN";
    }
}

DMMotorType string_to_dm_motor_type(const std::string& type_str) {
    if (type_str == "DM4310") return DMMotorType::DM4310;
    if (type_str == "DM6248") return DMMotorType::DM6248;
    if (type_str == "DM4340") return DMMotorType::DM4340;
    if (type_str == "DM10010L") return DMMotorType::DM10010L;
    throw std::invalid_argument("Invalid DM motor type: " + type_str);
}

// 辅助函数：转换 SafetyViolationType
std::string safety_violation_type_to_string(SafetyViolationType type) {
    switch (type) {
        case SafetyViolationType::POSITION_LIMIT: return "POSITION_LIMIT";
        case SafetyViolationType::VELOCITY_LIMIT: return "VELOCITY_LIMIT";
        case SafetyViolationType::TORQUE_LIMIT: return "TORQUE_LIMIT";
        case SafetyViolationType::TEMPERATURE_LIMIT: return "TEMPERATURE_LIMIT";
        case SafetyViolationType::COMMUNICATION_ERROR: return "COMMUNICATION_ERROR";
        case SafetyViolationType::MOTOR_ERROR: return "MOTOR_ERROR";
        case SafetyViolationType::EMERGENCY_STOP: return "EMERGENCY_STOP";
        case SafetyViolationType::WATCHDOG_TIMEOUT: return "WATCHDOG_TIMEOUT";
        case SafetyViolationType::SOFTWARE_ERROR: return "SOFTWARE_ERROR";
        default: return "UNKNOWN";
    }
}

// 辅助函数：创建时间戳
auto get_timestamp() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}

// Python 模块定义
PYBIND11_MODULE(ic_can_python, m) {
    m.doc() = "IC_CAN Python bindings - High Performance CAN Control Library for Robotic Arms";

    // ========== 异常类绑定 ==========

    py::register_exception<ic_can::USB2CANException>(m, "USB2CANException", PyExc_RuntimeError);

    // ========== 枚举类型绑定 ==========

    py::enum_<MotorType>(m, "MotorType")
        .value("DM_DAMIAO", MotorType::DM_DAMIAO)
        .value("HT_HIGH_TORQUE", MotorType::HT_HIGH_TORQUE)
        .value("SERVO", MotorType::SERVO)
        .def("from_string", &string_to_motor_type)
        .def("to_string", &motor_type_to_string);

    py::enum_<DMMotorType>(m, "DMMotorType")
        .value("DM4310", DMMotorType::DM4310)
        .value("DM6248", DMMotorType::DM6248)
        .value("DM4340", DMMotorType::DM4340)
        .value("DM10010L", DMMotorType::DM10010L)
        .def("from_string", &string_to_dm_motor_type)
        .def("to_string", &dm_motor_type_to_string);

    py::enum_<DMControlMode>(m, "DMControlMode")
        .value("MIT_MODE", DMControlMode::MIT_MODE)
        .value("POS_VEL_MODE", DMControlMode::POS_VEL_MODE)
        .value("VEL_MODE", DMControlMode::VEL_MODE)
        .value("POS_FORCE_MODE", DMControlMode::POS_FORCE_MODE);

    py::enum_<DMRegisterID>(m, "DMRegisterID")
        .value("UV_VALUE", DMRegisterID::UV_VALUE)
        .value("KT_VALUE", DMRegisterID::KT_VALUE)
        .value("OT_VALUE", DMRegisterID::OT_VALUE)
        .value("OC_VALUE", DMRegisterID::OC_VALUE)
        .value("ACC", DMRegisterID::ACC)
        .value("DEC", DMRegisterID::DEC)
        .value("MAX_SPD", DMRegisterID::MAX_SPD)
        .value("MST_ID", DMRegisterID::MST_ID)
        .value("ESC_ID", DMRegisterID::ESC_ID)
        .value("TIMEOUT", DMRegisterID::TIMEOUT)
        .value("CTRL_MODE", DMRegisterID::CTRL_MODE)
        .value("HW_VER", DMRegisterID::HW_VER)
        .value("SW_VER", DMRegisterID::SW_VER)
        .value("SN", DMRegisterID::SN)
        .value("PMAX", DMRegisterID::PMAX)
        .value("VMAX", DMRegisterID::VMAX)
        .value("TMAX", DMRegisterID::TMAX);

    py::enum_<SafetyViolationType>(m, "SafetyViolationType")
        .value("POSITION_LIMIT", SafetyViolationType::POSITION_LIMIT)
        .value("VELOCITY_LIMIT", SafetyViolationType::VELOCITY_LIMIT)
        .value("TORQUE_LIMIT", SafetyViolationType::TORQUE_LIMIT)
        .value("TEMPERATURE_LIMIT", SafetyViolationType::TEMPERATURE_LIMIT)
        .value("COMMUNICATION_ERROR", SafetyViolationType::COMMUNICATION_ERROR)
        .value("MOTOR_ERROR", SafetyViolationType::MOTOR_ERROR)
        .value("EMERGENCY_STOP", SafetyViolationType::EMERGENCY_STOP)
        .value("WATCHDOG_TIMEOUT", SafetyViolationType::WATCHDOG_TIMEOUT)
        .value("SOFTWARE_ERROR", SafetyViolationType::SOFTWARE_ERROR)
        .def("to_string", &safety_violation_type_to_string);

    // ========== 主控制器类绑定 ==========

    py::class_<ic_can::IC_CAN>(m, "IC_CAN")
        .def(py::init<const std::string&, bool>(),
             py::arg("device_sn") = "F561E08C892274DB09496BCC1102DBC5",
             py::arg("debug") = false,
             "Initialize IC_CAN controller")

        // 系统控制
        .def("initialize", &ic_can::IC_CAN::initialize,
             "Initialize the system and connect to hardware")
        .def("is_initialized", &ic_can::IC_CAN::is_initialized,
             "Check if the system is initialized")
        .def("enable_all", &ic_can::IC_CAN::enable_all,
             "Enable all motors")
        .def("disable_all", &ic_can::IC_CAN::disable_all,
             "Disable all motors")
        .def("emergency_stop", &ic_can::IC_CAN::emergency_stop,
             "Trigger emergency stop")
        .def("reset_emergency_stop", &ic_can::IC_CAN::reset_emergency_stop,
             "Reset emergency stop state")

        // 组件访问
        .def("get_arm", &ic_can::IC_CAN::get_arm,
             py::return_value_policy::reference_internal,
             "Get arm component")
        .def("get_gripper", &ic_can::IC_CAN::get_gripper,
             py::return_value_policy::reference_internal,
             "Get gripper component")
        .def("get_safety", &ic_can::IC_CAN::get_safety,
             py::return_value_policy::reference_internal,
             "Get safety module")

        // 状态读取
        .def("update_all_states", &ic_can::IC_CAN::update_all_states,
             "Update all motor states")
        .def("refresh_all", &ic_can::IC_CAN::refresh_all,
             "Refresh all states with safety checks")
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
        .def("set_joint_torques", &ic_can::IC_CAN::set_joint_torques,
             py::arg("torques"), py::arg("positions") = std::vector<double>{},
             py::arg("velocities") = std::vector<double>{},
             "Set joint torques")

        // 高频控制
        .def("start_high_frequency_control", &ic_can::IC_CAN::start_high_frequency_control,
             "Start high frequency control thread")
        .def("stop_high_frequency_control", &ic_can::IC_CAN::stop_high_frequency_control,
             "Stop high frequency control thread")
        .def("is_high_frequency_control_running", &ic_can::IC_CAN::is_high_frequency_control_running,
             "Check if high frequency control is running")

        // 配置接口
        .def("set_control_frequency", &ic_can::IC_CAN::set_control_frequency,
             py::arg("frequency_hz"),
             "Set control frequency in Hz")
        .def("get_control_frequency", &ic_can::IC_CAN::get_control_frequency,
             "Get current control frequency in Hz")

        // 诊断接口
        .def("get_system_status", &ic_can::IC_CAN::get_system_status,
             "Get system status information")
        .def("print_system_info", &ic_can::IC_CAN::print_system_info,
             "Print system information");

    // ========== 手臂组件绑定 ==========

    py::class_<ic_can::ArmComponent>(m, "ArmComponent")
        .def("get_positions", &ic_can::ArmComponent::get_positions,
             "Get all arm joint positions")
        .def("set_positions", &ic_can::ArmComponent::set_positions,
             py::arg("positions"), py::arg("velocities") = std::vector<double>{},
             py::arg("torques") = std::vector<double>{},
             "Set arm joint positions")
        .def("get_velocities", &ic_can::ArmComponent::get_velocities,
             "Get all arm joint velocities")
        .def("set_velocities", &ic_can::ArmComponent::set_velocities,
             py::arg("velocities"), py::arg("torques") = std::vector<double>{},
             "Set arm joint velocities")
        .def("get_torques", &ic_can::ArmComponent::get_torques,
             "Get all arm joint torques")
        .def("set_torques", &ic_can::ArmComponent::set_torques,
             py::arg("torques"),
             "Set arm joint torques")
        .def("mit_control", &ic_can::ArmComponent::mit_control,
             py::arg("positions"), py::arg("velocities"), py::arg("torques"),
             py::arg("kp_array"), py::arg("kd_array"),
             "MIT control for arm motors")
        .def("home_to_zero", &ic_can::ArmComponent::home_to_zero,
             py::arg("speed") = 0.5, py::arg("timeout") = 30.0,
             "Move arm to zero position")
        .def("stop_all_motion", &ic_can::ArmComponent::stop_all_motion,
             "Stop all arm motion")
        .def("print_arm_state", &ic_can::ArmComponent::print_arm_state,
             "Print arm state information")
        .def("test_arm_functionality", &ic_can::ArmComponent::test_arm_functionality,
             "Test arm functionality");

    // ========== 夹爪组件绑定 ==========

    py::class_<ic_can::GripperComponent>(m, "GripperComponent")
        .def("open", &ic_can::GripperComponent::open,
             py::arg("speed") = 0.5, py::arg("force") = 0.3,
             "Open gripper")
        .def("close", &ic_can::GripperComponent::close,
             py::arg("speed") = 0.5, py::arg("force") = 0.5,
             "Close gripper")
        .def("stop", &ic_can::GripperComponent::stop,
             "Stop gripper motion")
        .def("set_openness", &ic_can::GripperComponent::set_openness,
             py::arg("openness"), py::arg("speed") = 0.5, py::arg("force") = 0.3,
             "Set gripper openness (0.0-1.0)")
        .def("get_openness", &ic_can::GripperComponent::get_openness,
             "Get current gripper openness")
        .def("set_force", &ic_can::GripperComponent::set_force,
             py::arg("force"),
             "Set gripper force (0.0-1.0)")
        .def("get_current_force", &ic_can::GripperComponent::get_current_force,
             "Get current gripper force")
        .def("force_control", &ic_can::GripperComponent::force_control,
             py::arg("target_force"),
             "Force control mode")
        .def("get_positions", &ic_can::GripperComponent::get_positions,
             "Get gripper motor positions")
        .def("set_positions", &ic_can::GripperComponent::set_positions,
             py::arg("positions"), py::arg("velocities") = std::vector<double>{},
             "Set gripper motor positions")
        .def("set_servo_angle", &ic_can::GripperComponent::set_servo_angle,
             py::arg("angle"), py::arg("speed") = 0.5,
             "Set servo angle in degrees")
        .def("get_servo_angle", &ic_can::GripperComponent::get_servo_angle,
             "Get current servo angle in degrees")
        .def("is_fully_open", &ic_can::GripperComponent::is_fully_open,
             "Check if gripper is fully open")
        .def("is_fully_closed", &ic_can::GripperComponent::is_fully_closed,
             "Check if gripper is fully closed")
        .def("is_moving", &ic_can::GripperComponent::is_moving,
             "Check if gripper is moving")
        .def("is_grasping", &ic_can::GripperComponent::is_grasping,
             "Check if gripper is grasping an object")
        .def("perform_grasp_sequence", &ic_can::GripperComponent::perform_grasp_sequence,
             py::arg("target_openness"), py::arg("approach_speed"), py::arg("grasp_force"),
             py::arg("timeout") = 5.0,
             "Perform grasp sequence")
        .def("release_object", &ic_can::GripperComponent::release_object,
             py::arg("release_speed") = 0.8, py::arg("final_openness") = 1.0,
             "Release object")
        .def("auto_grasp", &ic_can::GripperComponent::auto_grasp,
             py::arg("max_force") = 0.6, py::arg("speed") = 0.4, py::arg("timeout") = 3.0,
             "Auto grasp with force feedback")
        .def("print_gripper_state", &ic_can::GripperComponent::print_gripper_state,
             "Print gripper state information")
        .def("test_gripper_functionality", &ic_can::GripperComponent::test_gripper_functionality,
             "Test gripper functionality");

    // ========== 安全模块绑定 ==========

    py::class_<ic_can::SafetyModule>(m, "SafetyModule")
        .def("trigger_emergency_stop", &ic_can::SafetyModule::trigger_emergency_stop,
             py::arg("reason") = "Manual emergency stop", py::arg("motor_id") = 0,
             "Trigger emergency stop")
        .def("reset_emergency_stop", &ic_can::SafetyModule::reset_emergency_stop,
             "Reset emergency stop state")
        .def("is_emergency_stopped", &ic_can::SafetyModule::is_emergency_stopped,
             "Check if emergency stop is active")
        .def("feed_watchdog", &ic_can::SafetyModule::feed_watchdog,
             "Feed the watchdog timer")
        .def("is_watchdog_timeout", &ic_can::SafetyModule::is_watchdog_timeout,
             "Check if watchdog has timed out")
        .def("get_active_violations", &ic_can::SafetyModule::get_active_violations,
             "Get active safety violations")
        .def("has_active_violations", &ic_can::SafetyModule::has_active_violations,
             "Check if there are active violations")
        .def("print_safety_status_report", &ic_can::SafetyModule::print_safety_status_report,
             "Print safety status report");

    // ========== 工具函数绑定 ==========

    m.def("get_timestamp", &get_timestamp,
         "Get current timestamp in milliseconds");

    m.def("motor_type_to_string", &motor_type_to_string,
         "Convert MotorType to string");

    m.def("string_to_motor_type", &string_to_motor_type,
         "Convert string to MotorType");

    m.def("dm_motor_type_to_string", &dm_motor_type_to_string,
         "Convert DMMotorType to string");

    m.def("string_to_dm_motor_type", &string_to_dm_motor_type,
         "Convert string to DMMotorType");

    // 版本信息
    m.attr("__version__") = "1.0.0";
    m.attr("__author__") = "IC_CAN Team";
    m.attr("__email__") = "openarm@enactic.ai";
    m.attr("__license__") = "Apache License 2.0";

    // 调试模式标志
    #ifdef DEVELOPMENT_MODE
    m.attr("_development_mode") = true;
    #else
    m.attr("_development_mode") = false;
    #endif
}