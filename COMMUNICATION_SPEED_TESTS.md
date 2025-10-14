# IC_CAN Communication Speed Tests

## 🚀 Purpose

Test the actual communication speed and frequency performance of the IC_CAN system with real hardware.

## 📋 Available Tests

### 1. **Communication Speed Test** (`communication_speed_test`)
- **Duration**: 10 seconds
- **Target**: Motors 1-6
- **Command**: MIT control with (pos=0, vel=0, torque=0, kp=0, kd=0)
- **Measures**:
  - Commands sent per second
  - Responses received per second
  - Total communication loop performance
  - Response success rate
  - Motor position changes

### 2. **High-Frequency Test** (`high_frequency_test`)
- **Duration**: 10 seconds maximum speed test + 5 seconds built-in 500Hz test
- **Target**: Motors 1-6
- **Command**: Maximum frequency MIT commands
- **Measures**:
  - Maximum achievable frequency
  - Success rate at maximum speed
  - Built-in 500Hz control performance

## 🔧 Compile Commands

```bash
# Communication Speed Test
g++ -std=c++17 -O2 \
  -I. -I./include -I./dm-tools/USB2FDCAN/SDK/C++/ubuntu/include \
  dm-tools/USB2FDCAN/SDK/C++/ubuntu/lib/usb_class.cpp.o \
  dm-tools/USB2FDCAN/SDK/C++/ubuntu/lib/crc.cpp.o \
  -lusb-1.0 -pthread \
  examples/communication_speed_test.cpp src/ic_can/core/ic_can.cpp \
  -o communication_speed_test

# High-Frequency Test
g++ -std=c++17 -O2 \
  -I. -I./include -I./dm-tools/USB2FDCAN/SDK/C++/ubuntu/include \
  dm-tools/USB2FDCAN/SDK/C++/ubuntu/lib/usb_class.cpp.o \
  dm-tools/USB2FDCAN/SDK/C++/ubuntu/lib/crc.cpp.o \
  -lusb-1.0 -pthread \
  examples/high_frequency_test.cpp src/ic_can/core/ic_can.cpp \
  -o high_frequency_test
```

## 🎯 How to Run

### **Test 1: Communication Speed (Recommended First)**
```bash
./communication_speed_test
```

**Output Example:**
```
Time (s) | Commands/s | Responses/s | Total Sent | Total Recv | Status
---------|------------|-------------|------------|------------|--------
     1.0 |      450.2 |       420.1 |        450 |        420 | ✅ FAST
     2.0 |      480.5 |       445.3 |        960 |        890 | ✅ FAST
     ...
```

### **Test 2: High-Frequency Maximum Speed**
```bash
./high_frequency_test
```

**Output Example:**
```
Time: 2.0s, Frequency: 520.3 Hz, Success Rate: 98.5%
Time: 4.0s, Frequency: 515.7 Hz, Success Rate: 97.9%
...
🏆 PERFORMANCE ASSESSMENT:
🚀 OUTSTANDING: Achieved 500Hz+ - ready for real-time control!
```

## 📊 What These Tests Measure

### **Communication Speed Test:**
- ✅ **Commands per second**: How many MIT commands can be sent
- ✅ **Responses per second**: How many motor responses are received
- ✅ **Success rate**: Percentage of successful communication
- ✅ **Motor position tracking**: Whether motors stay at zero position
- ✅ **Real-time performance**: Actual 100Hz+ capability verification

### **High-Frequency Test:**
- ✅ **Maximum frequency**: Push system to absolute limits
- ✅ **Burst performance**: Short-term maximum speed
- ✅ **Built-in 500Hz control**: Test the internal high-frequency loop
- ✅ **System stability**: Performance under maximum load

## 🎯 Success Criteria

### **Excellent Performance:**
- **Commands/s**: ≥ 500 Hz
- **Response Rate**: ≥ 90%
- **Success Rate**: ≥ 95%
- **Motor Stability**: Motors maintain zero position

### **Good Performance:**
- **Commands/s**: ≥ 200 Hz
- **Response Rate**: ≥ 70%
- **Success Rate**: ≥ 85%
- **Motor Stability**: Minor position drift acceptable

### **Needs Optimization:**
- **Commands/s**: < 100 Hz
- **Response Rate**: < 50%
- **Success Rate**: < 70%
- **Motor Stability**: Significant position drift

## 🔧 Test Parameters

### **MIT Command Being Sent:**
- **Position**: 0.0 rad (zero position)
- **Velocity**: 0.0 rad/s (no movement)
- **Torque**: 0.0 Nm (no force)
- **Kp**: 0.0 (no position gain)
- **Kd**: 0.0 (no velocity gain)

### **Target Motors:**
- **Motors 1-6**: Damiao motors (DM10010L, DM6248, DM4340, DM4310)
- **Motors 7-9**: Not included in speed test (HT motors have different protocol)

## 🚀 Ready to Test!

These tests will give you the actual performance numbers for your IC_CAN system. Run them with your hardware connected to see:

1. **Real communication frequency** (not theoretical)
2. **Actual response rates** from motors
3. **System stability under load**
4. **Readiness for 500Hz control applications**

**Start with `./communication_speed_test` for a comprehensive analysis!** 🎯