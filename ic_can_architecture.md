# IC_CAN 系统架构设计

## 项目概述

IC_CAN 是为 ic_arm 设计的高性能 CAN 控制管道，支持 9 个电机（8个手臂电机 + 1个夹爪电机）在 500Hz 频率下的实时控制。

## 系统架构图

```
┌─────────────────────────────────────────────────────────────────────────────────┐
│                            Python 应用层 (Application Layer)                      │
├─────────────────────────────────────────────────────────────────────────────────┤
│  IC_ARM_Control (高级控制功能)                                                    │
│  ┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐            │
│  │   轨迹规划       │    │   重力补偿       │    │   安全监控       │            │
│  │  Trajectory     │    │  Gravity        │    │   Safety        │            │
│  └─────────────────┘    └─────────────────┘    └─────────────────┘            │
└─────────────────────────────────────────────────────────────────────────────────┘
                                        │
                                        ▼
┌─────────────────────────────────────────────────────────────────────────────────┐
│                        Python 绑定层 (Python Binding Layer)                      │
├─────────────────────────────────────────────────────────────────────────────────┤
│                              PyBind11 接口                                       │
│  ┌─────────────────────────────────────────────────────────────────────────┐   │
│  │                      ic_can_python                                        │   │
│  │  ┌─────────────────┐            ┌─────────────────┐                    │   │
│  │  │  ArmManager     │            │  MotorManager   │                    │   │
│  │  │  (机械臂管理器)  │            │  (电机管理器)     │                    │   │
│  │  └─────────────────┘            └─────────────────┘                    │   │
│  └─────────────────────────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────────────────────┘
                                        │
                                        ▼
┌─────────────────────────────────────────────────────────────────────────────────┐
│                         C++ 核心控制层 (C++ Core Control Layer)                  │
├─────────────────────────────────────────────────────────────────────────────────┤
│                           IC_CAN 主控制器                                        │
│  ┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐            │
│  │  ArmComponent   │    │ GripperComponent │    │  SafetyModule   │            │
│  │  (手臂组件)      │    │  (夹爪组件)      │    │  (安全模块)      │            │
│  └─────────────────┘    └─────────────────┘    └─────────────────┘            │
└─────────────────────────────────────────────────────────────────────────────────┘
                                        │
                                        ▼
┌─────────────────────────────────────────────────────────────────────────────────┐
│                        设备管理层 (Device Management Layer)                     │
├─────────────────────────────────────────────────────────────────────────────────┤
│                         MotorDeviceCollection                                    │
│  ┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐            │
│  │   DMMotor       │    │   HTMotor       │    │  ServoMotor     │            │
│  │  (达妙电机1-6)   │    │  (高扭矩电机7-8) │    │  (舵机电机9)     │            │
│  └─────────────────┘    └─────────────────┘    └─────────────────┘            │
└─────────────────────────────────────────────────────────────────────────────────┘
                                        │
                                        ▼
┌─────────────────────────────────────────────────────────────────────────────────┐
│                         通信协议层 (Communication Protocol Layer)                │
├─────────────────────────────────────────────────────────────────────────────────┤
│  ┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐            │
│  │  DMProtocol     │    │  HTProtocol     │    │  ServoProtocol  │            │
│  │  (达妙协议)      │    │  (高扭矩协议)    │    │  (舵机协议)      │            │
│  └─────────────────┘    └─────────────────┘    └─────────────────┘            │
└─────────────────────────────────────────────────────────────────────────────────┘
                                        │
                                        ▼
┌─────────────────────────────────────────────────────────────────────────────────┐
│                        CAN 通信层 (CAN Communication Layer)                     │
├─────────────────────────────────────────────────────────────────────────────────┤
│  ┌─────────────────────────────────────────────────────────────────────────┐   │
│  │                     USB2CAN Adapter                                     │   │
│  │              (基于 dm-tools SDK)                                        │   │
│  └─────────────────────────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────────────────────┘
                                        │
                                        ▼
┌─────────────────────────────────────────────────────────────────────────────────┐
│                           硬件层 (Hardware Layer)                              │
├─────────────────────────────────────────────────────────────────────────────────┤
│  ┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐            │
│  │  DM10010L(m1)   │    │  DM6248(m2,m3)  │    │  DM4340(m4,m5)  │            │
│  └─────────────────┘    └─────────────────┘    └─────────────────┘            │
│  ┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐            │
│  │  DM4310(m6)     │    │  HT4438(m7,m8)  │    │  Servo(m9)      │            │
│  └─────────────────┘    └─────────────────┘    └─────────────────┘            │
└─────────────────────────────────────────────────────────────────────────────────┘
```

## 核心组件设计

### 1. **电机配置信息**

```cpp
// 电机配置 (基于 IC_ARM.py 中的配置)
struct MotorConfig {
    int motor_id;           // 电机ID (1-9)
    MotorType type;         // 电机类型
    uint32_t can_send_id;   // 发送CAN ID
    uint32_t can_recv_id;   // 接收CAN ID
    double kp;              // 位置增益
    double kd;              // 速度增益
    double max_torque;      // 最大扭矩
};

// 电机类型定义
enum class MotorType {
    DM_DAMIAO,     // 达妙电机
    HT_HIGH_TORQUE, // 高扭矩电机
    SERVO          // 舵机
};
```

### 2. **IC_CAN 主控制器**

```cpp
class IC_CAN {
public:
    IC_CAN(const std::string& device_sn, bool enable_debug = false);
    ~IC_CAN();

    // 系统控制
    bool initialize();
    bool enable_all();
    bool disable_all();
    bool emergency_stop();

    // 组件访问
    ArmComponent& get_arm() { return *arm_; }
    GripperComponent& get_gripper() { return *gripper_; }

    // 状态更新
    void update_all_states();
    void refresh_all();

    // 高频控制接口
    void send_commands_all();
    void receive_data_all(int timeout_us = 500);

private:
    std::unique_ptr<USB2CANAdapter> can_adapter_;
    std::unique_ptr<ArmComponent> arm_;
    std::unique_ptr<GripperComponent> gripper_;
    std::unique_ptr<SafetyModule> safety_;

    bool initialized_;
    std::atomic<bool> emergency_stop_flag_;
};
```

### 3. **组件化设计**

#### ArmComponent (手臂组件)
- 管理 6 个达妙电机 (m1-m6)
- 提供统一的位置、速度、扭矩控制接口
- 支持坐标变换和运动学计算

#### GripperComponent (夹爪组件)
- 管理 2 个高扭矩电机 (m7-m8) + 1 个舵机 (m9)
- 提供夹爪开合控制
- 支持力控制和位置控制

### 4. **设备管理层**

#### MotorDeviceCollection
```cpp
class MotorDeviceCollection {
public:
    void add_motor(std::shared_ptr<BaseMotor> motor);
    void remove_motor(int motor_id);
    BaseMotor* get_motor(int motor_id);

    // 批量操作
    void enable_all();
    void disable_all();
    void update_all_states();
    void send_commands_all();

    // 数据接收分发
    void dispatch_frame(const CANFrame& frame);

private:
    std::map<int, std::shared_ptr<BaseMotor>> motors_;
    std::mutex motors_mutex_;
};
```

### 5. **电机抽象层**

#### BaseMotor (电机基类)
```cpp
class BaseMotor {
public:
    BaseMotor(int motor_id, MotorType type, uint32_t send_id, uint32_t recv_id);
    virtual ~BaseMotor() = default;

    // 状态获取
    virtual double get_position() const = 0;
    virtual double get_velocity() const = 0;
    virtual double get_torque() const = 0;

    // 控制接口
    virtual bool set_command(double pos, double vel, double torque) = 0;
    virtual bool enable() = 0;
    virtual bool disable() = 0;

    // 状态更新
    virtual void update_state() = 0;
    virtual void process_response(const std::vector<uint8_t>& data) = 0;

protected:
    int motor_id_;
    MotorType type_;
    uint32_t can_send_id_;
    uint32_t can_recv_id_;

    // 状态变量
    std::atomic<double> position_{0.0};
    std::atomic<double> velocity_{0.0};
    std::atomic<double> torque_{0.0};
    std::atomic<bool> enabled_{false};
};
```

### 6. **通信协议层**

#### USB2CANAdapter
```cpp
class USB2CANAdapter {
public:
    USB2CANAdapter(const std::string& device_sn);
    ~USB2CANAdapter();

    // 连接管理
    bool connect();
    bool disconnect();
    bool is_connected() const;

    // CAN 通信
    bool send_frame(uint32_t can_id, const std::vector<uint8_t>& data);
    bool receive_frame(uint32_t& can_id, std::vector<uint8_t>& data, int timeout_us);

    // 批量操作
    void send_frames_batch(const std::vector<CANFrame>& frames);
    void receive_frames_batch(int timeout_us);

    // 回调注册
    void set_frame_callback(std::function<void(uint32_t, const std::vector<uint8_t>&)> callback);

private:
    // 基于 dm-tools SDK 的实现
    void* dm_device_;  // 指向 damiao::Motor_Control 的指针
    std::string device_sn_;
    std::atomic<bool> connected_{false};

    // 接收线程
    std::thread receive_thread_;
    std::atomic<bool> stop_receive_thread_{false};

    // 回调函数
    std::function<void(uint32_t, const std::vector<uint8_t>&)> frame_callback_;

    void receive_worker();
};
```

## 性能优化策略

### 1. **高频控制优化 (500Hz)**

#### 批量数据处理
```cpp
// 批量发送命令，减少 USB 通信开销
class BatchCommandProcessor {
public:
    void add_command(int motor_id, const MotorCommand& cmd);
    void send_all_commands();

private:
    std::vector<MotorCommand> command_buffer_;
    std::mutex buffer_mutex_;
};
```

#### 内存池管理
```cpp
// 预分配内存池，避免频繁内存分配
class MemoryPool {
public:
    static MemoryPool& instance();
    uint8_t* allocate_buffer(size_t size);
    void deallocate_buffer(uint8_t* buffer);

private:
    std::vector<std::unique_ptr<uint8_t[]>> buffers_;
    std::queue<uint8_t*> available_buffers_;
    std::mutex pool_mutex_;
};
```

### 2. **多线程架构**

```cpp
// 主控制线程 (500Hz)
void control_thread() {
    while (running_) {
        auto start_time = std::chrono::high_resolution_clock::now();

        // 1. 更新所有电机状态
        update_all_states();

        // 2. 执行控制算法
        execute_control_loop();

        // 3. 发送命令
        send_commands_all();

        // 4. 精确延时控制 (2ms = 500Hz)
        auto end_time = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
        auto sleep_time = std::chrono::microseconds(2000) - elapsed;

        if (sleep_time.count() > 0) {
            std::this_thread::sleep_for(sleep_time);
        }
    }
}

// 数据接收线程 (独立运行)
void receive_thread() {
    while (!stop_receive_thread_) {
        can_adapter_->receive_frames_batch(1000);  // 1ms timeout
    }
}
```

### 3. **状态缓存机制**

```cpp
class StateCache {
public:
    struct MotorState {
        double position;
        double velocity;
        double torque;
        std::chrono::steady_clock::time_point timestamp;
        bool valid;
    };

    // 获取缓存状态，避免重复读取
    MotorState get_cached_state(int motor_id, int max_age_us = 1000);

    // 更新缓存
    void update_state(int motor_id, double pos, double vel, double torque);

private:
    std::array<MotorState, 10> state_cache_;  // 支持1-9号电机
    std::mutex cache_mutex_;
};
```

## Python 绑定设计

### 1. **PyBind11 接口**

```cpp
// ic_can_python.cpp
PYBIND11_MODULE(ic_can_python, m) {
    m.doc() = "IC_CAN Python bindings";

    py::class_<IC_CAN>(m, "IC_CAN")
        .def(py::init<std::string, bool>())
        .def("initialize", &IC_CAN::initialize)
        .def("enable_all", &IC_CAN::enable_all)
        .def("disable_all", &IC_CAN::disable_all)
        .def("emergency_stop", &IC_CAN::emergency_stop)
        .def("update_all_states", &IC_CAN::update_all_states)
        .def("get_joint_positions", &IC_CAN::get_joint_positions)
        .def("set_joint_positions", &IC_CAN::set_joint_positions)
        .def("set_joint_torques", &IC_CAN::set_joint_torques);

    py::class_<ArmComponent>(m, "ArmComponent")
        .def("get_positions", &ArmComponent::get_positions)
        .def("set_positions", &ArmComponent::set_positions)
        .def("home_to_zero", &ArmComponent::home_to_zero);

    py::class_<GripperComponent>(m, "GripperComponent")
        .def("open", &GripperComponent::open)
        .def("close", &GripperComponent::close)
        .def("set_force", &GripperComponent::set_force);
}
```

### 2. **Python 高级接口**

```python
# ic_can/__init__.py
class ICARM:
    def __init__(self, device_sn="F561E08C892274DB09496BCC1102DBC5", debug=False):
        self.controller = ic_can_python.IC_CAN(device_sn, debug)

    def connect(self):
        """连接并初始化系统"""
        return self.controller.initialize()

    def enable(self):
        """使能所有电机"""
        return self.controller.enable_all()

    def disable(self):
        """禁用所有电机"""
        return self.controller.disable_all()

    def get_joint_positions(self):
        """获取所有关节位置 (弧度)"""
        return self.controller.get_joint_positions()

    def set_joint_positions(self, positions, torques=None):
        """设置关节位置"""
        if torques is not None:
            return self.controller.set_joint_positions(positions, torques)
        return self.controller.set_joint_positions(positions)

    # 兼容 IC_ARM.py 的接口
    def get_positions_degrees(self):
        """获取关节位置 (度)"""
        positions_rad = self.get_joint_positions()
        return np.degrees(positions_rad)

    def set_joint_positions_degrees(self, positions_deg, torques=None):
        """设置关节位置 (度)"""
        positions_rad = np.radians(positions_deg)
        return self.set_joint_positions(positions_rad, torques)
```

## 安全设计

### 1. **SafetyModule (安全模块)**

```cpp
class SafetyModule {
public:
    // 安全检查
    bool check_position_limits(const std::vector<double>& positions);
    bool check_velocity_limits(const std::vector<double>& velocities);
    bool check_torque_limits(const std::vector<double>& torques);
    bool check_temperature_limits();

    // 紧急停止
    void trigger_emergency_stop(const std::string& reason);
    void reset_emergency_stop();

    // 安全状态查询
    bool is_emergency_stopped() const;
    std::vector<std::string> get_active_violations() const;

private:
    std::atomic<bool> emergency_stop_{false};
    std::vector<std::string> active_violations_;
    std::mutex violations_mutex_;

    // 安全限制 (根据电机规格设置)
    std::vector<double> position_limits_;
    std::vector<double> velocity_limits_;
    std::vector<double> torque_limits_;
    std::vector<double> temperature_limits_;
};
```

### 2. **看门狗机制**

```cpp
class Watchdog {
public:
    Watchdog(std::chrono::milliseconds timeout) : timeout_(timeout) {}

    void feed();
    bool is_expired() const;
    void reset();

private:
    std::chrono::steady_clock::time_point last_feed_;
    std::chrono::milliseconds timeout_;
};
```

## 构建系统

### 1. **CMake 配置**

```cmake
# CMakeLists.txt
cmake_minimum_required(VERSION 3.12)
project(ic_can)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找依赖
find_package(pybind11 REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBUSB REQUIRED libusb-1.0)

# 包含目录
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/dm-tools/USB2FDCAN/SDK/C++/arm/include
    ${LIBUSB_INCLUDE_DIRS}
)

# 源文件
file(GLOB_RECURSE SOURCES
    "src/*.cpp"
    "src/*.h"
)

# 动态库
add_library(ic_can_core SHARED ${SOURCES})
target_link_libraries(ic_can_core
    ${LIBUSB_LIBRARIES}
    pthread
)

# Python 绑定
pybind11_add_module(ic_can_python
    python/ic_can_python.cpp
    $<TARGET_OBJECTS:ic_can_core>
)

target_link_libraries(ic_can_python PRIVATE ic_can_core)

# 安装规则
install(TARGETS ic_can_core ic_can_python
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)
```

### 2. **测试框架**

```cpp
// tests/test_basic.cpp
#include <gtest/gtest.h>
#include "ic_can/ic_can.hpp"

class IC_CANTest : public ::testing::Test {
protected:
    void SetUp() override {
        ic_can = std::make_unique<IC_CAN>("test_device", true);
    }

    void TearDown() override {
        if (ic_can && ic_can->is_initialized()) {
            ic_can->disable_all();
        }
    }

    std::unique_ptr<IC_CAN> ic_can;
};

TEST_F(IC_CANTest, Initialization) {
    EXPECT_TRUE(ic_can->initialize());
    EXPECT_TRUE(ic_can->is_initialized());
}

TEST_F(IC_CANTest, MotorControl) {
    ASSERT_TRUE(ic_can->initialize());
    EXPECT_TRUE(ic_can->enable_all());

    // 测试位置控制
    std::vector<double> positions = {0.1, 0.2, 0.3, 0.4, 0.5, 0.6};
    EXPECT_TRUE(ic_can->set_joint_positions(positions));

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    auto current_positions = ic_can->get_joint_positions();
    EXPECT_EQ(current_positions.size(), 6);
}
```

## 部署和使用

### 1. **编译和安装**

```bash
# 构建项目
mkdir build && cd build
cmake ..
make -j$(nproc)

# 安装到系统
sudo make install

# 或者使用 pip 安装 Python 包
cd ../python
pip install -e .
```

### 2. **使用示例**

```python
#!/usr/bin/env python3
import ic_can
import numpy as np
import time

# 创建控制器实例
arm = ic_can.ICARM(debug=True)

try:
    # 连接和初始化
    if not arm.connect():
        print("Failed to connect")
        exit(1)

    # 使能所有电机
    if not arm.enable():
        print("Failed to enable motors")
        exit(1)

    print("System initialized successfully!")

    # 简单的位置控制测试
    target_positions = np.array([10, 20, 30, 40, 50, 60])  # 度
    print(f"Moving to position: {target_positions}°")

    arm.set_joint_positions_degrees(target_positions)
    time.sleep(2.0)

    # 读取当前位置
    current_pos = arm.get_positions_degrees()
    print(f"Current position: {current_pos}°")

    # 回到零位
    print("Moving to zero position...")
    arm.set_joint_positions_degrees(np.zeros(6))
    time.sleep(2.0)

except KeyboardInterrupt:
    print("Interrupted by user")
finally:
    # 安全关闭
    arm.disable()
    print("System shutdown complete")
```

## 开发计划

### 阶段一：核心框架 (Week 1-2)
- [ ] 基础架构搭建
- [ ] USB2CAN 适配器实现
- [ ] 电机抽象基类设计
- [ ] 基本的达妙电机协议实现

### 阶段二：功能实现 (Week 3-4)
- [ ] 9 个电机的完整支持
- [ ] 安全模块实现
- [ ] Python 绑定接口
- [ ] 基础测试用例

### 阶段三：性能优化 (Week 5-6)
- [ ] 500Hz 高频控制优化
- [ ] 多线程架构完善
- [ ] 内存管理和缓存优化
- [ ] 性能基准测试

### 阶段四：集成测试 (Week 7-8)
- [ ] 与现有 IC_ARM.py 系统集成
- [ ] 完整的机械臂功能测试
- [ ] 长期稳定性测试
- [ ] 文档和部署指南