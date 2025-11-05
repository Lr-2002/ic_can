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

#include "ic_can/core/can_communication_factory.hpp"
#include "ic_can/core/can_protocol_interface.hpp"
#include "ic_can/core/zlg_canfd_communication_adapter.hpp"
#include "ic_can/core/simulation_communication_adapter.hpp"
#include <iostream>
#include <chrono>
#include <algorithm>
#include <iomanip>
#include <thread>

namespace ic_can {

// Static member initialization
CommunicationFactoryConfig CANCommunicationFactory::default_config_;
bool CANCommunicationFactory::default_config_set_ = false;

// Backend registry implementation
std::map<CommunicationBackend, std::function<std::unique_ptr<CANCommunicationInterface>(const CommunicationFactoryConfig&)>>
    CommunicationBackendRegistry::registered_backends_;
std::map<CommunicationBackend, std::string> CommunicationBackendRegistry::backend_names_;

// ===================================
// MAIN FACTORY METHODS
// ===================================

std::unique_ptr<CANCommunicationInterface> CANCommunicationFactory::create_adapter(
    const CommunicationFactoryConfig& config) {

    std::cout << "🏭 Creating CAN communication adapter (auto-detect mode)..." << std::endl;

    // Try backends in order of preference
    for (auto backend : config.fallback_backends) {
        if (is_backend_available(backend, config)) {
            std::cout << "✅ Selected backend: " << backend_to_string(backend) << std::endl;
            return create_adapter(backend, config);
        }
    }

    std::cout << "❌ No available backends found" << std::endl;
    return nullptr;
}

std::unique_ptr<CANCommunicationInterface> CANCommunicationFactory::create_adapter(
    CommunicationBackend backend,
    const CommunicationFactoryConfig& config) {

    std::cout << "🏭 Creating CAN communication adapter: " << backend_to_string(backend) << std::endl;

    switch (backend) {
        case CommunicationBackend::ZLG_CANFD:
            return create_zlg_canfd_adapter_internal(config);

        case CommunicationBackend::DM_TOOLS:
            return create_dm_tools_adapter_internal(config);

        case CommunicationBackend::SIMULATION:
            return create_simulation_adapter_internal(config);

        case CommunicationBackend::AUTO:
            return create_adapter(config);

        default:
            std::cout << "❌ Unknown backend type: " << static_cast<int>(backend) << std::endl;
            return nullptr;
    }
}

std::vector<CommunicationBackendInfo> CANCommunicationFactory::get_available_backends(
    const CommunicationFactoryConfig& config) {

    std::vector<CommunicationBackendInfo> backends;

    // Check each backend type
    std::vector<CommunicationBackend> backend_types = {
        CommunicationBackend::ZLG_CANFD,
        CommunicationBackend::DM_TOOLS,
        CommunicationBackend::SIMULATION
    };

    for (auto backend : backend_types) {
        bool available = is_backend_available(backend, config);
        std::string device_info = "";

        if (available) {
            switch (backend) {
                case CommunicationBackend::ZLG_CANFD: {
                    auto adapter = create_zlg_canfd_adapter_internal(config);
                    if (adapter && adapter->initialize()) {
                        device_info = "Available - ZLG CAN FD adapter";
                        adapter->shutdown();
                    }
                    break;
                }

                case CommunicationBackend::DM_TOOLS:
                    device_info = "Available - DM Tools USB2CAN adapter";
                    break;

                case CommunicationBackend::SIMULATION:
                    device_info = "Available - Simulation mode";
                    break;

                default:
                    break;
            }
        }

        backends.push_back(create_backend_info(backend, available, device_info));
    }

    return backends;
}

bool CANCommunicationFactory::is_backend_available(
    CommunicationBackend backend,
    const CommunicationFactoryConfig& config) {

    switch (backend) {
        case CommunicationBackend::ZLG_CANFD:
            return detect_zlg_canfd(config);

        case CommunicationBackend::DM_TOOLS:
            return detect_dm_tools(config);

        case CommunicationBackend::SIMULATION:
            return false; // Simulation disabled - force real hardware

        default:
            return false;
    }
}

std::map<std::string, bool> CANCommunicationFactory::test_backends(
    const CommunicationFactoryConfig& config,
    uint32_t test_duration_ms) {

    std::map<std::string, bool> test_results;

    std::cout << "🧪 Testing available CAN communication backends..." << std::endl;

    auto backends = get_available_backends(config);
    for (const auto& backend_info : backends) {
        std::string backend_name = backend_to_string(backend_info.type);

        std::cout << "\nTesting " << backend_name << "..." << std::endl;

        bool test_passed = false;
        try {
            auto adapter = create_adapter(backend_info.type, config);
            if (adapter && adapter->initialize()) {
                // Simple test: send a few frames
                uint32_t frames_sent = 0;
                auto start_time = std::chrono::high_resolution_clock::now();

                while (std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::high_resolution_clock::now() - start_time).count() < test_duration_ms) {

                    CANFrame test_frame;
                    test_frame.id = 0x100;
                    test_frame.data.resize(8);
                    for (int i = 0; i < 8; i++) {
                        test_frame.data[i] = static_cast<uint8_t>(frames_sent + i);
                    }

                    if (adapter->send_frame(test_frame)) {
                        frames_sent++;
                    }

                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                }

                auto stats = adapter->get_statistics();
                test_passed = (frames_sent > 0 && stats.send_errors == 0);

                std::cout << "   Frames sent: " << frames_sent << std::endl;
                std::cout << "   Send errors: " << stats.send_errors << std::endl;
                std::cout << "   Result: " << (test_passed ? "✅ PASSED" : "❌ FAILED") << std::endl;

                adapter->shutdown();
            }
        } catch (const std::exception& e) {
            std::cout << "   ❌ Exception: " << e.what() << std::endl;
            test_passed = false;
        }

        test_results[backend_name] = test_passed;
    }

    return test_results;
}

std::map<std::string, std::map<std::string, double>> CANCommunicationFactory::compare_backend_performance(
    const CommunicationFactoryConfig& config,
    uint32_t test_duration_ms) {

    std::map<std::string, std::map<std::string, double>> performance_results;

    std::cout << "📊 Comparing backend performance (" << test_duration_ms << "ms test)..." << std::endl;

    auto backends = get_available_backends(config);
    for (const auto& backend_info : backends) {
        if (!backend_info.available) {
            continue;
        }

        std::string backend_name = backend_to_string(backend_info.type);
        std::map<std::string, double> results;

        std::cout << "\n📈 Testing " << backend_name << " performance..." << std::endl;

        try {
            auto adapter = create_adapter(backend_info.type, config);
            if (adapter && adapter->initialize()) {
                // Performance test
                uint32_t frames_sent = 0;
                uint32_t frames_received = 0;
                auto start_time = std::chrono::high_resolution_clock::now();

                while (std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::high_resolution_clock::now() - start_time).count() < test_duration_ms) {

                    CANFrame test_frame;
                    test_frame.id = 0x100 + (frames_sent % 10);
                    test_frame.data.resize(8);
                    for (int i = 0; i < 8; i++) {
                        test_frame.data[i] = static_cast<uint8_t>(frames_sent + i);
                    }

                    if (adapter->send_frame(test_frame)) {
                        frames_sent++;
                    }

                    // Note: receive_frame is not part of the basic interface
                    // In a real implementation, frames would be received via callback

                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                }

                auto end_time = std::chrono::high_resolution_clock::now();
                auto total_time = std::chrono::duration<double>(end_time - start_time).count();

                auto stats = adapter->get_statistics();

                results["frames_sent"] = static_cast<double>(frames_sent);
                results["frames_received"] = static_cast<double>(frames_received);
                results["send_frequency"] = frames_sent / total_time;
                results["receive_frequency"] = frames_received / total_time;
                results["success_rate"] = (frames_sent > 0) ? (100.0 * frames_sent / (frames_sent + stats.send_errors)) : 0.0;
                results["avg_latency_us"] = 0.0;  // Not available in basic stats
                results["max_latency_us"] = 0.0;  // Not available in basic stats

                std::cout << "   Send Frequency: " << std::fixed << std::setprecision(1)
                          << results["send_frequency"] << " Hz" << std::endl;
                std::cout << "   Success Rate: " << std::fixed << std::setprecision(1)
                          << results["success_rate"] << "%" << std::endl;
                std::cout << "   Avg Latency: " << std::fixed << std::setprecision(2)
                          << results["avg_latency_us"] << " μs" << std::endl;

                adapter->shutdown();
            }
        } catch (const std::exception& e) {
            std::cout << "   ❌ Exception: " << e.what() << std::endl;
        }

        performance_results[backend_name] = results;
    }

    // Print comparison summary
    std::cout << "\n📊 Performance Comparison Summary:" << std::endl;
    std::cout << std::string(80, '=') << std::endl;
    std::cout << std::left << std::setw(15) << "Backend"
              << std::setw(12) << "Send Freq"
              << std::setw(12) << "Success"
              << std::setw(12) << "Avg Latency"
              << std::setw(15) << "Max Latency" << std::endl;
    std::cout << std::string(80, '-') << std::endl;

    for (const auto& [backend_name, results] : performance_results) {
        std::cout << std::left << std::setw(15) << backend_name
                  << std::setw(12) << std::fixed << std::setprecision(1) << results.at("send_frequency")
                  << std::setw(11) << std::fixed << std::setprecision(1) << results.at("success_rate") << "%"
                  << std::setw(12) << std::fixed << std::setprecision(1) << results.at("avg_latency_us")
                  << std::setw(15) << std::fixed << std::setprecision(1) << results.at("max_latency_us") << " μs"
                  << std::endl;
    }

    return performance_results;
}

CommunicationBackend CANCommunicationFactory::get_recommended_backend(
    const CommunicationFactoryConfig& config) {

    // Priority order for backend selection
    if (is_backend_available(CommunicationBackend::ZLG_CANFD, config)) {
        return CommunicationBackend::ZLG_CANFD;
    }

    if (is_backend_available(CommunicationBackend::DM_TOOLS, config)) {
        return CommunicationBackend::DM_TOOLS;
    }

    return CommunicationBackend::SIMULATION;
}

void CANCommunicationFactory::set_default_config(const CommunicationFactoryConfig& config) {
    default_config_ = config;
    default_config_set_ = true;
    std::cout << "✅ Default communication factory configuration set" << std::endl;
}

CommunicationFactoryConfig CANCommunicationFactory::get_default_config() {
    if (!default_config_set_) {
        // Set sensible defaults
        default_config_ = CommunicationFactoryConfig{};
        default_config_set_ = true;
    }
    return default_config_;
}

// ===================================
// CONVENIENCE FACTORY METHODS
// ===================================

std::unique_ptr<CANCommunicationInterface> CANCommunicationFactory::create_zlg_canfd_adapter(
    const std::string& device_serial,
    int channel_index,
    bool can_fd_mode) {

    CommunicationFactoryConfig config = get_default_config();
    config.preferred_backend = CommunicationBackend::ZLG_CANFD;
    config.device_serial = device_serial;
    config.preferred_channel = channel_index;
    config.zlg_can_fd_mode = can_fd_mode;

    return create_zlg_canfd_adapter_internal(config);
}

std::unique_ptr<CANCommunicationInterface> CANCommunicationFactory::create_dm_tools_adapter(
    const std::string& device_serial) {

    CommunicationFactoryConfig config = get_default_config();
    config.preferred_backend = CommunicationBackend::DM_TOOLS;
    config.device_serial = device_serial;

    return create_dm_tools_adapter_internal(config);
}

std::unique_ptr<CANCommunicationInterface> CANCommunicationFactory::create_simulation_adapter(
    bool loopback) {

    CommunicationFactoryConfig config = get_default_config();
    config.preferred_backend = CommunicationBackend::SIMULATION;

    return create_simulation_adapter_internal(config);
}

std::unique_ptr<CANCommunicationInterface> CANCommunicationFactory::create_adapter_by_name(
    const std::string& backend_name,
    const CommunicationFactoryConfig& config) {

    CommunicationBackend backend = string_to_backend(backend_name);
    return create_adapter(backend, config);
}

// ===================================
// INTERNAL FACTORY METHODS
// ===================================

std::unique_ptr<CANCommunicationInterface> CANCommunicationFactory::create_zlg_canfd_adapter_internal(
    const CommunicationFactoryConfig& config) {

    ZLGCanFDConfig zlg_config;
    zlg_config.device_type = USBCANFD_200U;
    zlg_config.device_index = 0;
    zlg_config.channel_index = (config.preferred_channel >= 0) ?
                               static_cast<uint32_t>(config.preferred_channel) : 0;
    zlg_config.auto_channel_selection = (config.preferred_channel < 0);
    zlg_config.arbitration_baud = config.zlg_arbitration_baud;
    zlg_config.data_baud = config.zlg_data_baud;
    zlg_config.can_fd_mode = config.zlg_can_fd_mode;
    zlg_config.library_path = config.zlg_library_path;
    zlg_config.receive_timeout_ms = config.receive_timeout_ms;
    zlg_config.send_timeout_ms = config.send_timeout_ms;
    zlg_config.debug = config.debug_enabled;

    auto adapter = std::make_unique<ZLGCanFDCommunicationAdapter>(zlg_config);

    if (config.debug_enabled) {
        std::cout << "🔧 ZLG CAN FD adapter created with configuration:" << std::endl;
        std::cout << "   Device Type: " << zlg_config.device_type << std::endl;
        std::cout << "   Channel: " << zlg_config.channel_index << std::endl;
        std::cout << "   CAN FD Mode: " << (zlg_config.can_fd_mode ? "Enabled" : "Disabled") << std::endl;
        std::cout << "   Auto Channel: " << (zlg_config.auto_channel_selection ? "Enabled" : "Disabled") << std::endl;
    }

    return std::move(adapter);
}

std::unique_ptr<CANCommunicationInterface> CANCommunicationFactory::create_dm_tools_adapter_internal(
    const CommunicationFactoryConfig& config) {

    // TODO: Implement DM Tools adapter creation
    // This would create a USB2CANCommunicationAdapter using the dm-tools SDK
    std::cout << "❌ DM Tools adapter not yet implemented" << std::endl;
    return nullptr;
}

std::unique_ptr<CANCommunicationInterface> CANCommunicationFactory::create_simulation_adapter_internal(
    const CommunicationFactoryConfig& config) {

    auto adapter = std::make_unique<SimulationCommunicationAdapter>(config.debug_enabled);

    if (config.debug_enabled) {
        std::cout << "🔧 Simulation adapter created" << std::endl;
    }

    return std::move(adapter);
}

// ===================================
// BACKEND DETECTION AND TESTING
// ===================================

bool CANCommunicationFactory::detect_zlg_canfd(const CommunicationFactoryConfig& config) {
    try {
        ZLGCanFDConfig zlg_config;
        zlg_config.debug = config.debug_enabled;
        zlg_config.library_path = config.zlg_library_path;

        auto adapter = std::make_unique<ZLGCanFDCommunicationAdapter>(zlg_config);
        bool detected = adapter->initialize("");

        if (detected) {
            adapter->shutdown();
            if (config.debug_enabled) {
                std::cout << "✅ ZLG CAN FD backend detected" << std::endl;
            }
        }

        return detected;
    } catch (const std::exception& e) {
        if (config.debug_enabled) {
            std::cout << "❌ ZLG CAN FD detection failed: " << e.what() << std::endl;
        }
        return false;
    }
}

bool CANCommunicationFactory::detect_dm_tools(const CommunicationFactoryConfig& config) {
    // TODO: Implement DM Tools detection
    // This would check for dm-tools libraries and hardware availability
    return false;
}

// ===================================
// UTILITY METHODS
// ===================================

std::string CANCommunicationFactory::backend_to_string(CommunicationBackend backend) {
    switch (backend) {
        case CommunicationBackend::AUTO: return "auto";
        case CommunicationBackend::DM_TOOLS: return "dm_tools";
        case CommunicationBackend::ZLG_CANFD: return "zlg_canfd";
        case CommunicationBackend::SIMULATION: return "simulation";
        default: return "unknown";
    }
}

CommunicationBackend CANCommunicationFactory::string_to_backend(const std::string& backend_name) {
    if (backend_name == "auto" || backend_name == "AUTO") {
        return CommunicationBackend::AUTO;
    } else if (backend_name == "dm_tools" || backend_name == "DM_TOOLS") {
        return CommunicationBackend::DM_TOOLS;
    } else if (backend_name == "zlg" || backend_name == "zlg_canfd" || backend_name == "ZLG_CANFD") {
        return CommunicationBackend::ZLG_CANFD;
    } else if (backend_name == "simulation" || backend_name == "SIMULATION") {
        return CommunicationBackend::SIMULATION;
    } else {
        return CommunicationBackend::AUTO; // Default to auto
    }
}

CommunicationBackendInfo CANCommunicationFactory::create_backend_info(
    CommunicationBackend backend,
    bool available,
    const std::string& device_info) {

    CommunicationBackendInfo info;
    info.type = backend;
    info.name = backend_to_string(backend);
    info.available = available;
    info.device_info = device_info;

    // Set capabilities based on backend type
    switch (backend) {
        case CommunicationBackend::ZLG_CANFD:
            info.description = "ZLG CAN FD adapter with multi-channel support";
            info.capabilities = {
                {"can_fd", "true"},
                {"max_channels", "16"},
                {"max_data_length", "64"},
                {"configurable_baud", "true"},
                {"hardware_filtering", "true"}
            };
            break;

        case CommunicationBackend::DM_TOOLS:
            info.description = "DM Tools USB2CAN adapter (legacy)";
            info.capabilities = {
                {"can_fd", "false"},
                {"max_channels", "1"},
                {"max_data_length", "8"},
                {"configurable_baud", "false"},
                {"hardware_filtering", "false"}
            };
            break;

        case CommunicationBackend::SIMULATION:
            info.description = "Simulation mode for testing";
            info.capabilities = {
                {"can_fd", "true"},
                {"max_channels", "unlimited"},
                {"max_data_length", "64"},
                {"configurable_baud", "true"},
                {"hardware_filtering", "true"}
            };
            break;

        default:
            info.description = "Unknown backend";
            break;
    }

    return info;
}

// ===================================
// BACKEND REGISTRY IMPLEMENTATION
// ===================================

bool CommunicationBackendRegistry::register_backend(
    CommunicationBackend backend,
    const std::string& name,
    const std::string& description,
    std::function<std::unique_ptr<CANCommunicationInterface>(const CommunicationFactoryConfig&)> factory_func) {

    registered_backends_[backend] = factory_func;
    backend_names_[backend] = name;

    std::cout << "✅ Registered backend: " << name << std::endl;
    return true;
}

bool CommunicationBackendRegistry::unregister_backend(CommunicationBackend backend) {
    if (registered_backends_.erase(backend)) {
        backend_names_.erase(backend);
        // Simple backend to string conversion for registry
        auto backend_to_string_simple = [](CommunicationBackend b) -> std::string {
            switch (b) {
                case CommunicationBackend::AUTO: return "auto";
                case CommunicationBackend::DM_TOOLS: return "dm_tools";
                case CommunicationBackend::ZLG_CANFD: return "zlg_canfd";
                case CommunicationBackend::SIMULATION: return "simulation";
                default: return "unknown";
            }
        };
        std::cout << "🗑️ Unregistered backend: " << backend_to_string_simple(backend) << std::endl;
        return true;
    }
    return false;
}

std::map<CommunicationBackend, std::string> CommunicationBackendRegistry::get_registered_backends() {
    return backend_names_;
}

// ===================================
// CONVENIENCE FUNCTION
// ===================================

std::unique_ptr<CANCommunicationInterface> create_can_adapter(
    const std::string& backend_type,
    const std::string& device_serial) {

    CommunicationFactoryConfig config;
    config.device_serial = device_serial;

    // Set backend-specific defaults
    if (backend_type == "zlg" || backend_type == "zlg_canfd") {
        config.preferred_backend = CommunicationBackend::ZLG_CANFD;
        config.zlg_can_fd_mode = true;
    } else if (backend_type == "dm_tools") {
        config.preferred_backend = CommunicationBackend::DM_TOOLS;
    } else if (backend_type == "simulation") {
        config.preferred_backend = CommunicationBackend::SIMULATION;
    }

    return CANCommunicationFactory::create_adapter(config);
}

} // namespace ic_can