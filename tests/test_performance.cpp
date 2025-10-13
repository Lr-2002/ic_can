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

#include <gtest/gtest.h>
#include <chrono>
#include <thread>
#include <vector>
#include <atomic>
#include <mutex>
#include <iostream>
#include <fstream>
#include <iomanip>

#include <ic_can/core/ic_can.hpp>
#include <ic_can/motors/base_motor.hpp>
#include <ic_can/motors/dm_motor.hpp>
#include <ic_can/protocols/usb2can_adapter.hpp>
#include <ic_can/core/arm_component.hpp>
#include <ic_can/core/gripper_component.hpp>

using namespace ic_can;
using namespace std::chrono;

class PerformanceTest : public ::testing::Test {
protected:
    void SetUp() override {
        std::cout << "\n=== Performance Test Setup ===" << std::endl;
        test_results.clear();
    }

    void TearDown() override {
        std::cout << "=== Performance Test Teardown ===" << std::endl;
        save_performance_results();
    }

    struct PerformanceResult {
        std::string test_name;
        double avg_frequency_hz;
        double min_frequency_hz;
        double max_frequency_hz;
        double avg_latency_us;
        double max_latency_us;
        double cpu_usage_percent;
        size_t total_iterations;
        duration<double> total_time;
    };

    std::vector<PerformanceResult> test_results;
    std::mutex results_mutex;

    void add_result(const PerformanceResult& result) {
        std::lock_guard<std::mutex> lock(results_mutex);
        test_results.push_back(result);
    }

    void save_performance_results() {
        if (test_results.empty()) return;

        std::ofstream file("performance_results.csv");
        if (!file.is_open()) {
            std::cerr << "Failed to open performance results file" << std::endl;
            return;
        }

        file << "Test Name,Avg Frequency (Hz),Min Frequency (Hz),Max Frequency (Hz),"
             << "Avg Latency (us),Max Latency (us),CPU Usage (%),Total Iterations,Total Time (s)\n";

        for (const auto& result : test_results) {
            file << result.test_name << ","
                 << std::fixed << std::setprecision(2) << result.avg_frequency_hz << ","
                 << result.min_frequency_hz << ","
                 << result.max_frequency_hz << ","
                 << std::setprecision(3) << result.avg_latency_us << ","
                 << result.max_latency_us << ","
                 << result.cpu_usage_percent << ","
                 << result.total_iterations << ","
                 << std::setprecision(6) << result.total_time.count() << "\n";
        }

        file.close();
        std::cout << "Performance results saved to performance_results.csv" << std::endl;
    }

    double measure_cpu_usage() {
        // Simple CPU usage estimation
        // In a real implementation, this would use platform-specific APIs
        return 0.0; // Placeholder
    }
};

// Test motor update performance
TEST_F(PerformanceTest, MotorUpdatePerformance) {
    std::cout << "Testing motor update performance..." << std::endl;

    const int num_motors = 6;
    const int test_iterations = 10000;
    std::vector<std::unique_ptr<DMMotor>> motors;

    // Create motors
    for (int i = 0; i < num_motors; ++i) {
        motors.push_back(std::make_unique<DMMotor>(
            i + 1, DMMotorType::DM4310, "DM4310_" + std::to_string(i)
        ));
    }

    std::vector<double> frequencies;
    std::vector<double> latencies;

    auto start_time = high_resolution_clock::now();

    for (int i = 0; i < test_iterations; ++i) {
        auto iteration_start = high_resolution_clock::now();

        // Update all motors
        for (auto& motor : motors) {
            motor->update_state(
                0.1 * i,      // position
                0.01,         // velocity
                0.05,         // torque
                25.0 + i % 10, // temperature
                0             // error
            );
        }

        auto iteration_end = high_resolution_clock::now();
        auto iteration_time = duration_cast<microseconds>(iteration_end - iteration_start);
        latencies.push_back(iteration_time.count());

        if (i > 0) {
            double frequency = 1e6 / iteration_time.count();
            frequencies.push_back(frequency);
        }
    }

    auto end_time = high_resolution_clock::now();
    auto total_time = duration_cast<milliseconds>(end_time - start_time);

    // Calculate statistics
    double avg_frequency = 0;
    double min_frequency = *std::min_element(frequencies.begin(), frequencies.end());
    double max_frequency = *std::max_element(frequencies.begin(), frequencies.end());

    for (double freq : frequencies) {
        avg_frequency += freq;
    }
    avg_frequency /= frequencies.size();

    double avg_latency = 0;
    double max_latency = *std::max_element(latencies.begin(), latencies.end());

    for (double latency : latencies) {
        avg_latency += latency;
    }
    avg_latency /= latencies.size();

    PerformanceResult result;
    result.test_name = "Motor Update Performance";
    result.avg_frequency_hz = avg_frequency;
    result.min_frequency_hz = min_frequency;
    result.max_frequency_hz = max_frequency;
    result.avg_latency_us = avg_latency;
    result.max_latency_us = max_latency;
    result.cpu_usage_percent = measure_cpu_usage();
    result.total_iterations = test_iterations;
    result.total_time = duration<double>(total_time);

    add_result(result);

    std::cout << "Motor Update Performance Results:" << std::endl;
    std::cout << "  Average Frequency: " << avg_frequency << " Hz" << std::endl;
    std::cout << "  Min Frequency: " << min_frequency << " Hz" << std::endl;
    std::cout << "  Max Frequency: " << max_frequency << " Hz" << std::endl;
    std::cout << "  Average Latency: " << avg_latency << " μs" << std::endl;
    std::cout << "  Max Latency: " << max_latency << " μs" << std::endl;
    std::cout << "  Total Time: " << total_time.count() << " ms" << std::endl;

    // Performance assertions
    EXPECT_GT(avg_frequency, 10000) << "Motor update should be fast enough for high-frequency control";
    EXPECT_LT(avg_latency, 100) << "Motor update latency should be under 100μs";
}

// Test CAN frame processing performance
TEST_F(PerformanceTest, CANFrameProcessingPerformance) {
    std::cout << "Testing CAN frame processing performance..." << std::endl;

    const int test_frames = 10000;
    const int frame_size = 8;

    std::vector<CANFrame> frames;
    for (int i = 0; i < test_frames; ++i) {
        CANFrame frame;
        frame.can_id = 0x200 + (i % 10);
        frame.dlc = frame_size;
        frame.data.resize(frame_size);
        for (int j = 0; j < frame_size; ++j) {
            frame.data[j] = static_cast<uint8_t>(i + j);
        }
        frame.timestamp = high_resolution_clock::now();
        frames.push_back(frame);
    }

    std::vector<double> processing_times;
    auto start_time = high_resolution_clock::now();

    for (const auto& frame : frames) {
        auto process_start = high_resolution_clock::now();

        // Simulate frame processing
        volatile uint32_t can_id = frame.can_id;
        volatile uint8_t dlc = frame.dlc;
        volatile uint32_t data_sum = 0;

        for (uint8_t byte : frame.data) {
            data_sum += byte;
        }

        // Prevent optimization
        (void)can_id; (void)dlc; (void)data_sum;

        auto process_end = high_resolution_clock::now();
        auto process_time = duration_cast<nanoseconds>(process_end - process_start);
        processing_times.push_back(process_time.count());
    }

    auto end_time = high_resolution_clock::now();
    auto total_time = duration_cast<microseconds>(end_time - start_time);

    double avg_process_time = 0;
    double max_process_time = *std::max_element(processing_times.begin(), processing_times.end());

    for (double time : processing_times) {
        avg_process_time += time;
    }
    avg_process_time /= processing_times.size();

    double throughput = (double)test_frames / total_time.count() * 1e6; // frames per second

    std::cout << "CAN Frame Processing Performance Results:" << std::endl;
    std::cout << "  Frames Processed: " << test_frames << std::endl;
    std::cout << "  Average Process Time: " << avg_process_time << " ns" << std::endl;
    std::cout << "  Max Process Time: " << max_process_time << " ns" << std::endl;
    std::cout << "  Throughput: " << throughput << " frames/s" << std::endl;
    std::cout << "  Total Time: " << total_time.count() << " μs" << std::endl;

    PerformanceResult result;
    result.test_name = "CAN Frame Processing";
    result.avg_frequency_hz = throughput;
    result.min_frequency_hz = throughput; // Single measurement
    result.max_frequency_hz = throughput;
    result.avg_latency_us = avg_process_time / 1000.0;
    result.max_latency_us = max_process_time / 1000.0;
    result.cpu_usage_percent = measure_cpu_usage();
    result.total_iterations = test_frames;
    result.total_time = duration<double>(total_time);

    add_result(result);

    // Performance assertions
    EXPECT_GT(throughput, 100000) << "Should process at least 100k frames per second";
    EXPECT_LT(avg_process_time, 10000) << "Average processing time should be under 10μs";
}

// Test high-frequency control loop performance
TEST_F(PerformanceTest, HighFrequencyControlLoop) {
    std::cout << "Testing high-frequency control loop performance..." << std::endl;

    const double target_frequency = 500.0; // Hz
    const int test_duration_ms = 2000; // 2 seconds
    const int expected_iterations = static_cast<int>(target_frequency * test_duration_ms / 1000.0);

    std::atomic<bool> running{true};
    std::vector<double> loop_times;
    std::mutex loop_times_mutex;

    auto control_loop = [&]() {
        auto last_time = high_resolution_clock::now();

        while (running) {
            auto current_time = high_resolution_clock::now();
            auto loop_time = duration_cast<microseconds>(current_time - last_time);

            {
                std::lock_guard<std::mutex> lock(loop_times_mutex);
                loop_times.push_back(loop_time.count());
            }

            // Simulate control computations
            std::vector<double> positions(7, 0.1);
            std::vector<double> velocities(7, 0.01);
            std::vector<double> torques(7, 0.05);

            // Simple computation
            for (size_t i = 0; i < positions.size(); ++i) {
                positions[i] += velocities[i] * 0.001; // 1ms timestep
                torques[i] = 0.1 * (1.0 - positions[i]);
            }

            last_time = current_time;

            // Control frequency limiting
            auto elapsed = duration_cast<microseconds>(current_time - last_time);
            auto target_period = static_cast<int>(1e6 / target_frequency);
            if (elapsed.count() < target_period) {
                std::this_thread::sleep_for(microseconds(target_period - elapsed.count()));
            }
        }
    };

    // Start control thread
    std::thread control_thread(control_loop);

    // Run for specified duration
    std::this_thread::sleep_for(milliseconds(test_duration_ms));

    // Stop control thread
    running = false;
    control_thread.join();

    // Analyze performance
    std::lock_guard<std::mutex> lock(loop_times_mutex);

    ASSERT_GT(loop_times.size(), expected_iterations * 0.8) << "Should achieve at least 80% of target frequency";

    std::vector<double> frequencies;
    for (double loop_time : loop_times) {
        if (loop_time > 0) {
            frequencies.push_back(1e6 / loop_time);
        }
    }

    double avg_frequency = 0;
    double min_frequency = *std::min_element(frequencies.begin(), frequencies.end());
    double max_frequency = *std::max_element(frequencies.begin(), frequencies.end());

    for (double freq : frequencies) {
        avg_frequency += freq;
    }
    avg_frequency /= frequencies.size();

    double avg_loop_time = 0;
    double max_loop_time = *std::max_element(loop_times.begin(), loop_times.end());

    for (double time : loop_times) {
        avg_loop_time += time;
    }
    avg_loop_time /= loop_times.size();

    std::cout << "High-Frequency Control Loop Results:" << std::endl;
    std::cout << "  Target Frequency: " << target_frequency << " Hz" << std::endl;
    std::cout << "  Actual Frequency: " << avg_frequency << " Hz" << std::endl;
    std::cout << "  Frequency Range: [" << min_frequency << ", " << max_frequency << "] Hz" << std::endl;
    std::cout << "  Average Loop Time: " << avg_loop_time << " μs" << std::endl;
    std::cout << "  Max Loop Time: " << max_loop_time << " μs" << std::endl;
    std::cout << "  Total Iterations: " << loop_times.size() << std::endl;

    PerformanceResult result;
    result.test_name = "High-Frequency Control Loop";
    result.avg_frequency_hz = avg_frequency;
    result.min_frequency_hz = min_frequency;
    result.max_frequency_hz = max_frequency;
    result.avg_latency_us = avg_loop_time;
    result.max_latency_us = max_loop_time;
    result.cpu_usage_percent = measure_cpu_usage();
    result.total_iterations = loop_times.size();
    result.total_time = duration<double>(milliseconds(test_duration_ms));

    add_result(result);

    // Performance assertions
    EXPECT_GT(avg_frequency, target_frequency * 0.8) << "Should achieve at least 80% of target frequency";
    EXPECT_LT(avg_loop_time, 2000) << "Loop time should be under 2ms for 500Hz control";
}

// Test memory allocation performance
TEST_F(PerformanceTest, MemoryAllocationPerformance) {
    std::cout << "Testing memory allocation performance..." << std::endl;

    const int num_allocations = 10000;
    const size_t allocation_size = 1024; // 1KB

    std::vector<double> allocation_times;

    for (int i = 0; i < num_allocations; ++i) {
        auto alloc_start = high_resolution_clock::now();

        // Allocate and initialize memory
        auto buffer = std::make_unique<uint8_t[]>(allocation_size);
        for (size_t j = 0; j < allocation_size; ++j) {
            buffer[j] = static_cast<uint8_t>(i + j);
        }

        auto alloc_end = high_resolution_clock::now();
        auto alloc_time = duration_cast<nanoseconds>(alloc_end - alloc_start);
        allocation_times.push_back(alloc_time.count());

        // Use the buffer to prevent optimization
        volatile uint8_t sum = 0;
        for (size_t j = 0; j < allocation_size; ++j) {
            sum += buffer[j];
        }
        (void)sum; // Suppress unused variable warning
    }

    double avg_alloc_time = 0;
    double max_alloc_time = *std::max_element(allocation_times.begin(), allocation_times.end());

    for (double time : allocation_times) {
        avg_alloc_time += time;
    }
    avg_alloc_time /= allocation_times.size();

    double allocation_rate = (double)num_allocations / (avg_alloc_time * 1e-9);

    std::cout << "Memory Allocation Performance Results:" << std::endl;
    std::cout << "  Allocations: " << num_allocations << std::endl;
    std::cout << "  Allocation Size: " << allocation_size << " bytes" << std::endl;
    std::cout << "  Average Allocation Time: " << avg_alloc_time << " ns" << std::endl;
    std::cout << "  Max Allocation Time: " << max_alloc_time << " ns" << std::endl;
    std::cout << "  Allocation Rate: " << allocation_rate << " allocations/s" << std::endl;

    PerformanceResult result;
    result.test_name = "Memory Allocation";
    result.avg_frequency_hz = allocation_rate;
    result.min_frequency_hz = allocation_rate;
    result.max_frequency_hz = allocation_rate;
    result.avg_latency_us = avg_alloc_time / 1000.0;
    result.max_latency_us = max_alloc_time / 1000.0;
    result.cpu_usage_percent = measure_cpu_usage();
    result.total_iterations = num_allocations;
    result.total_time = duration<double>(microseconds(static_cast<int64_t>(num_allocations * avg_alloc_time / 1000.0)));

    add_result(result);

    // Performance assertions
    EXPECT_GT(allocation_rate, 100000) << "Should achieve at least 100k allocations per second";
    EXPECT_LT(avg_alloc_time, 10000) << "Average allocation time should be under 10μs";
}

// Test multi-threaded performance
TEST_F(PerformanceTest, MultiThreadedPerformance) {
    std::cout << "Testing multi-threaded performance..." << std::endl;

    const int num_threads = 4;
    const int iterations_per_thread = 5000;
    const size_t shared_data_size = 1000;

    std::vector<std::thread> threads;
    std::vector<std::vector<double>> thread_times(num_threads);
    std::atomic<int> completed_threads{0};

    // Shared data structure
    std::vector<double> shared_data(shared_data_size, 0.0);
    std::mutex shared_data_mutex;

    auto worker_function = [&](int thread_id) {
        auto thread_start = high_resolution_clock::now();

        for (int i = 0; i < iterations_per_thread; ++i) {
            auto iteration_start = high_resolution_clock::now();

            // Read from shared data
            double sum = 0.0;
            {
                std::lock_guard<std::mutex> lock(shared_data_mutex);
                for (double value : shared_data) {
                    sum += value;
                }
            }

            // Write to shared data
            {
                std::lock_guard<std::mutex> lock(shared_data_mutex);
                for (size_t j = 0; j < shared_data.size(); ++j) {
                    shared_data[j] = sum / (i + 1);
                }
            }

            auto iteration_end = high_resolution_clock::now();
            auto iteration_time = duration_cast<microseconds>(iteration_end - iteration_start);
            thread_times[thread_id].push_back(iteration_time.count());
        }

        auto thread_end = high_resolution_clock::now();
        auto thread_time = duration_cast<milliseconds>(thread_end - thread_start);

        completed_threads++;
        std::cout << "Thread " << thread_id << " completed in " << thread_time.count() << " ms" << std::endl;
    };

    // Start worker threads
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back(worker_function, i);
    }

    // Wait for all threads to complete
    for (auto& thread : threads) {
        thread.join();
    }

    // Calculate overall statistics
    std::vector<double> all_times;
    for (const auto& thread_time : thread_times) {
        all_times.insert(all_times.end(), thread_time.begin(), thread_time.end());
    }

    double avg_time = 0;
    double max_time = *std::max_element(all_times.begin(), all_times.end());

    for (double time : all_times) {
        avg_time += time;
    }
    avg_time /= all_times.size();

    double overall_throughput = (double)(num_threads * iterations_per_thread) / (avg_time * 1e-6);

    std::cout << "Multi-Threaded Performance Results:" << std::endl;
    std::cout << "  Threads: " << num_threads << std::endl;
    std::cout << "  Iterations per Thread: " << iterations_per_thread << std::endl;
    std::cout << "  Total Iterations: " << num_threads * iterations_per_thread << std::endl;
    std::cout << "  Average Iteration Time: " << avg_time << " μs" << std::endl;
    std::cout << "  Max Iteration Time: " << max_time << " μs" << std::endl;
    std::cout << "  Overall Throughput: " << overall_throughput << " iterations/s" << std::endl;

    PerformanceResult result;
    result.test_name = "Multi-Threaded Performance";
    result.avg_frequency_hz = overall_throughput;
    result.min_frequency_hz = overall_throughput;
    result.max_frequency_hz = overall_throughput;
    result.avg_latency_us = avg_time;
    result.max_latency_us = max_time;
    result.cpu_usage_percent = measure_cpu_usage();
    result.total_iterations = num_threads * iterations_per_thread;
    result.total_time = duration<double>(microseconds(static_cast<int64_t>(num_threads * iterations_per_thread * avg_time)));

    add_result(result);

    // Performance assertions
    EXPECT_GT(overall_throughput, 50000) << "Should achieve reasonable multi-threaded throughput";
    EXPECT_LT(avg_time, 1000) << "Average iteration time should be under 1ms";
}

int main(int argc, char** argv) {
    std::cout << "=== IC_CAN Performance Tests ===" << std::endl;
    std::cout << "Testing system performance under various load conditions" << std::endl;
    std::cout << "Results will be saved to performance_results.csv" << std::endl;
    std::cout << "================================" << std::endl;

    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}