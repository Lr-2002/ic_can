// ========== Clean HT Motor Implementation ==========
// Only two protocols:
// 1. 48-byte frame commands for setting positions
// 2. 12-byte refresh commands for requesting positions

/**
 * @brief Clean HT motor protocol implementation
 *
 * This replaces all old HT methods with just the two protocols you specified:
 * - send_ht_frame_command() - 48-byte frames for pose control
 * - send_ht_refresh_command() - 12-byte refresh for position requests
 */

#pragma once

#include <vector>
#include <iostream>
#include "ic_can/core/wrist_component.hpp"

namespace ic_can {

class CleanHTMotorProtocol {
public:
    /**
     * @brief Send 48-byte frame command to set HT motor positions
     * @param wrist Reference to wrist component
     * @param m7_position Motor 7 position in radians
     * @param m7_velocity Motor 7 velocity in rad/s
     * @param m7_torque Motor 7 torque in Nm
     * @param m8_position Motor 8 position in radians
     * @param m8_velocity Motor 8 velocity in rad/s
     * @param m8_torque Motor 8 torque in Nm
     * @param kp Position gain
     * @param kd Velocity gain
     * @return Success status
     */
    static bool send_ht_frame_command(WristComponent& wrist,
                                     double m7_position, double m7_velocity, double m7_torque,
                                     double m8_position, double m8_velocity, double m8_torque,
                                     double kp, double kd) {
        std::cout << "📤 Sending 48-byte HT frame commands to both motors" << std::endl;

        // Use wrist component to send 48-byte frames to both motors
        bool success = wrist.send_ht_frame_both_motors(
            m7_position, m7_velocity, m7_torque,
            m8_position, m8_velocity, m8_torque,
            kp, kd);

        std::cout << "   ✅ HT frame commands sent - Success: " << (success ? "YES" : "NO") << std::endl;
        return success;
    }

    /**
     * @brief Send 12-byte refresh command to request HT motor positions
     * @param wrist Reference to wrist component
     * @return Success status
     */
    static bool send_ht_refresh_command(WristComponent& wrist) {
        std::cout << "📤 Sending 12-byte HT refresh commands to both motors" << std::endl;

        // Send 12-byte refresh commands to both HT motors
        bool success7 = wrist.send_refresh_command(7);  // Motor 7
        bool success8 = wrist.send_refresh_command(8);  // Motor 8

        std::cout << "   ✅ HT refresh commands sent - M7: " << (success7 ? "YES" : "NO")
                  << ", M8: " << (success8 ? "YES" : "NO") << std::endl;

        return success7 && success8;
    }
};

} // namespace ic_can