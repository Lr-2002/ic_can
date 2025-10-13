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

#pragma once

#include "base_motor.hpp"
#include <cstdint>
#include <vector>
#include <map>

namespace ic_can {

// 达妙电机类型 (参考 dm-tools SDK)
enum class DMMotorType {
    DM3507 = 0,
    DM4310 = 1,
    DM4310_48V = 2,
    DM4340 = 3,
    DM4340_48V = 4,
    DM6006 = 5,
    DM6248 = 6,
    DM8006 = 7,
    DM8009 = 8,
    DM10010L = 9,
    DM10010 = 10,
    DMH3510 = 11,
    DMH6215 = 12,
    DMS3519 = 13,
    DMG6220 = 14
};

// 达妙电机控制模式
enum class DMControlMode {
    MIT_MODE = 0x000,      // MIT 控制模式
    POS_VEL_MODE = 0x100,  // 位置速度模式
    VEL_MODE = 0x200,      // 速度模式
    POS_FORCE_MODE = 0x300  // 位置力矩模式
};

// 达妙电机寄存器 ID (参考 dm-tools)
enum class DMRegisterID : uint8_t {
    UV_VALUE = 0,      // 电压值
    KT_VALUE = 1,      // 力矩常数
    OT_VALUE = 2,      // 过温保护
    OC_VALUE = 3,      // 过流保护
    ACC = 4,           // 加速度
    DEC = 5,           // 减速度
    MAX_SPD = 6,       // 最大速度
    MST_ID = 7,        // 主站ID
    ESC_ID = 8,        // 从站ID
    TIMEOUT = 9,       // 超时设置
    CTRL_MODE = 10,    // 控制模式
    HW_VER = 13,       // 硬件版本
    SW_VER = 14,       // 软件版本
    SN = 15,           // 序列号
    PMAX = 21,         // 最大位置
    VMAX = 22,         // 最大速度
    TMAX = 23          // 最大力矩
};

// 达妙电机命令类型
enum class DMCommand : uint8_t {
    ENABLE = 0xFC,      // 使能电机
    DISABLE = 0xFD,     // 禁用电机
    SET_ZERO = 0xFE,    // 设置零位
    REFRESH = 0xFF,     // 刷新状态
    MIT_CONTROL = 0xFF  // MIT 控制 (与其他命令共用)
};

/**
 * @brief 达妙电机类
 *
 * 实现达妙电机的完整控制接口，支持 MIT 控制、位置控制、速度控制等
 */
class DMMotor : public BaseMotor {
public:
    /**
     * @brief 构造函数
     * @param motor_id 电机ID
     * @param motor_type 达妙电机类型
     * @param can_send_id 发送CAN ID
     * @param can_recv_id 接收CAN ID
     */
    DMMotor(int motor_id, DMMotorType motor_type,
            uint32_t can_send_id, uint32_t can_recv_id);

    /**
     * @brief 析构函数
     */
    virtual ~DMMotor() = default;

    // ========== 基础控制接口 ==========

    /**
     * @brief 使能电机
     * @return 操作是否成功
     */
    bool enable() override;

    /**
     * @brief 禁用电机
     * @return 操作是否成功
     */
    bool disable() override;

    /**
     * @brief 设置零位
     * @return 操作是否成功
     */
    bool set_zero() override;

    /**
     * @brief 设置电机命令
     * @param command 电机命令
     * @return 操作是否成功
     */
    bool set_command(const MotorCommand& command) override;

    /**
     * @brief 设置位置命令
     * @param position 目标位置 (弧度)
     * @param velocity 目标速度 (弧度/秒)
     * @param torque 目标力矩 (牛顿·米)
     * @return 操作是否成功
     */
    bool set_position(double position, double velocity = 0.0, double torque = 0.0) override;

    /**
     * @brief 设置速度命令
     * @param velocity 目标速度 (弧度/秒)
     * @param torque 目标力矩 (牛顿·米)
     * @return 操作是否成功
     */
    bool set_velocity(double velocity, double torque = 0.0) override;

    /**
     * @brief 设置力矩命令
     * @param torque 目标力矩 (牛顿·米)
     * @return 操作是否成功
     */
    bool set_torque(double torque) override;

    // ========== MIT 控制接口 ==========

    /**
     * @brief MIT 控制
     * @param position 目标位置 (弧度)
     * @param velocity 目标速度 (弧度/秒)
     * @param torque 目标力矩 (牛顿·米)
     * @param kp 位置增益
     * @param kd 速度增益
     * @return 操作是否成功
     */
    bool mit_control(double position, double velocity, double torque,
                   double kp, double kd) override;

    // ========== 参数查询接口 ==========

    /**
     * @brief 读取电机参数
     * @param register_id 寄存器ID
     * @return 参数值，失败返回 NaN
     */
    double read_parameter(DMRegisterID register_id);

    /**
     * @brief 写入电机参数
     * @param register_id 寄存器ID
     * @param value 参数值
     * @return 操作是否成功
     */
    bool write_parameter(DMRegisterID register_id, float value);

    /**
     * @brief 刷新电机状态
     */
    void refresh();

    // ========== 状态更新接口 ==========

    /**
     * @brief 更新电机状态
     */
    void update_state() override;

    /**
     * @brief 处理接收到的数据
     * @param data 接收到的数据
     * @return 处理是否成功
     */
    bool process_response(const std::vector<uint8_t>& data) override;

    /**
     * @brief 获取需要发送的命令数据
     * @return CAN帧数据
     */
    std::vector<uint8_t> get_command_data() const override;

    // ========== 配置接口 ==========

    /**
     * @brief 获取达妙电机类型
     * @return 电机类型
     */
    DMMotorType get_dm_motor_type() const { return dm_motor_type_; }

    /**
     * @brief 设置控制模式
     * @param mode 控制模式
     * @return 操作是否成功
     */
    bool set_control_mode(DMControlMode mode);

    /**
     * @brief 获取控制模式
     * @return 当前控制模式
     */
    DMControlMode get_control_mode() const { return control_mode_; }

    // ========== 静态工具方法 ==========

    /**
     * @brief 获取电机类型的默认参数
     * @param motor_type 电机类型
     * @return 限制参数
     */
    static MotorLimits get_default_limits(DMMotorType motor_type);

    /**
     * @brief 弧度转换为电机内部单位
     * @param angle_rad 弧度
     * @return 内部单位
     */
    static double angle_to_internal_units(double angle_rad);

    /**
     * @brief 电机内部单位转换为弧度
     * @param internal_units 内部单位
     * @return 弧度
     */
    static double internal_units_to_angle(double internal_units);

private:
    // ========== 私有成员变量 ==========

    DMMotorType dm_motor_type_;         // 达妙电机类型
    DMControlMode control_mode_;         // 控制模式

    // MIT 控制参数
    double mit_kp_;                     // MIT 位置增益
    double mit_kd_;                     // MIT 速度增益

    // 命令缓存
    std::vector<uint8_t> command_buffer_; // 待发送的命令数据
    bool has_pending_command_;           // 是否有待发送的命令

    // 参数缓存
    std::map<DMRegisterID, float> parameter_cache_; // 参数缓存
    bool parameter_cache_valid_;        // 参数缓存是否有效

    // ========== 私有方法 ==========

    /**
     * @brief 创建使能命令
     * @return 命令数据
     */
    std::vector<uint8_t> create_enable_command();

    /**
     * @brief 创建禁用命令
     * @return 命令数据
     */
    std::vector<uint8_t> create_disable_command();

    /**
     * @brief 创建设置零位命令
     * @return 命令数据
     */
    std::vector<uint8_t> create_set_zero_command();

    /**
     * @brief 创建刷新命令
     * @return 命令数据
     */
    std::vector<uint8_t> create_refresh_command();

    /**
     * @brief 创建 MIT 控制命令
     * @param position 位置
     * @param velocity 速度
     * @param torque 力矩
     * @param kp 位置增益
     * @param kd 速度增益
     * @return 命令数据
     */
    std::vector<uint8_t> create_mit_command(double position, double velocity,
                                           double torque, double kp, double kd);

    /**
     * @brief 创建参数查询命令
     * @param register_id 寄存器ID
     * @return 命令数据
     */
    std::vector<uint8_t> create_read_param_command(DMRegisterID register_id);

    /**
     * @brief 创建参数写入命令
     * @param register_id 寄存器ID
     * @param value 参数值
     * @return 命令数据
     */
    std::vector<uint8_t> create_write_param_command(DMRegisterID register_id, float value);

    /**
     * @brief 解析状态响应数据
     * @param data 响应数据
     * @return 解析是否成功
     */
    bool parse_state_response(const std::vector<uint8_t>& data);

    /**
     * @brief 解析参数响应数据
     * @param data 响应数据
     * @param value 输出参数值
     * @return 解析是否成功
     */
    bool parse_parameter_response(const std::vector<uint8_t>& data, float& value);

    /**
     * @brief 浮点数转换为整数
     * @param value 浮点数值
     * @param min 最小值
     * @param max 最大值
     * @param bits 位数
     * @return 整数值
     */
    static uint16_t float_to_uint(double x, double min, double max, int bits);

    /**
     * @brief 整数转换为浮点数
     * @param x 整数值
     * @param min 最小值
     * @param max 最大值
     * @param bits 位数
     * @return 浮点数值
     */
    static double uint_to_float(uint16_t x, double min, double max, int bits);

    /**
     * @brief 获取电机类型的参数范围
     * @param motor_type 电机类型
     * @param p_max 输出最大位置
     * @param v_max 输出最大速度
     * @param t_max 输出最大力矩
     */
    static void get_motor_limits(DMMotorType motor_type,
                                double& p_max, double& v_max, double& t_max);

    /**
     * @brief 打印调试信息
     * @param message 调试信息
     */
    void debug_print(const std::string& message);
};

} // namespace ic_can