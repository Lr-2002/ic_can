#!/usr/bin/env python3
# Copyright 2025 IC_CAN Team
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""
IC_CAN Python Performance Tests
Comprehensive performance testing and benchmarking of the Python interface.
"""

import sys
import time
import threading
import logging
import csv
import json
import statistics
import psutil
import os
from typing import List, Dict, Any, Tuple
from dataclasses import dataclass, asdict
from concurrent.futures import ThreadPoolExecutor
import matplotlib.pyplot as plt
import numpy as np

# Configure logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s'
)
logger = logging.getLogger(__name__)

try:
    from ic_can_python import IC_CAN
    from ic_can.core import IC_CANController
    NATIVE_AVAILABLE = True
    logger.info("IC_CAN native module loaded successfully")
except ImportError as e:
    logger.warning(f"IC_CAN native module not available: {e}")
    logger.warning("Running in simulation mode")
    NATIVE_AVAILABLE = False

    # Mock class for simulation mode
    class IC_CAN:
        def __init__(self, device_sn="mock", debug=False):
            self.device_sn = device_sn
            self.debug = debug
            self._initialized = False
            self._high_freq_running = False
            self._control_frequency = 500.0

        def initialize(self):
            self._initialized = True
            return True

        def is_initialized(self):
            return self._initialized

        def get_joint_positions(self):
            return [0.0] * 7

        def get_joint_velocities(self):
            return [0.0] * 7

        def get_joint_torques(self):
            return [0.0] * 7

        def set_joint_positions(self, positions, velocities=None, torques=None):
            pass

        def set_joint_torques(self, torques, positions=None, velocities=None):
            pass

        def update_all_states(self):
            pass

        def start_high_frequency_control(self):
            self._high_freq_running = True

        def stop_high_frequency_control(self):
            self._high_freq_running = False

        def is_high_frequency_control_running(self):
            return self._high_freq_running

        def get_control_frequency(self):
            return self._control_frequency

        def set_control_frequency(self, freq):
            self._control_frequency = freq

        def get_system_status(self):
            return {"status": "mock"}

        def print_system_info(self):
            pass

    class IC_CANController:
        def __init__(self, device_sn="mock", debug=False):
            self.controller = IC_CAN(device_sn, debug)
            self._initialized = False

        def initialize(self):
            return self.controller.initialize()

        def is_initialized(self):
            return self.controller.is_initialized()


@dataclass
class PerformanceMetric:
    """Performance metric data structure"""
    test_name: str
    operation: str
    target_frequency: float
    actual_frequency: float
    avg_latency_ms: float
    min_latency_ms: float
    max_latency_ms: float
    std_latency_ms: float
    success_rate: float
    cpu_usage_percent: float
    memory_usage_mb: float
    total_operations: int
    duration_seconds: float
    timestamp: str


class PerformanceMonitor:
    """Performance monitoring utilities"""

    def __init__(self):
        self.process = psutil.Process(os.getpid())
        self.start_time = None
        self.start_memory = None

    def start_monitoring(self):
        """Start performance monitoring"""
        self.start_time = time.time()
        self.start_memory = self.process.memory_info().rss / 1024 / 1024  # MB

    def get_current_stats(self) -> Dict[str, float]:
        """Get current performance statistics"""
        current_memory = self.process.memory_info().rss / 1024 / 1024  # MB
        cpu_percent = self.process.cpu_percent()

        return {
            'memory_usage_mb': current_memory,
            'memory_increase_mb': current_memory - (self.start_memory or 0),
            'cpu_usage_percent': cpu_percent,
            'elapsed_time': time.time() - (self.start_time or time.time())
        }


class IC_CANPerformanceTester:
    """IC_CAN performance testing framework"""

    def __init__(self, device_sn: str = "F561E08C892274DB09496BCC1102DBC5"):
        self.device_sn = device_sn if NATIVE_AVAILABLE else "mock_device"
        self.controller = None
        self.high_level_controller = None
        self.monitor = PerformanceMonitor()
        self.results = []

    def setup(self):
        """Set up test environment"""
        logger.info(f"Setting up performance tester with device: {self.device_sn}")

        self.controller = IC_CAN(self.device_sn, debug=False)
        if not self.controller.initialize():
            raise RuntimeError("Failed to initialize IC_CAN controller")

        if NATIVE_AVAILABLE:
            self.high_level_controller = IC_CANController(self.device_sn, debug=False)
            self.high_level_controller.initialize()

        logger.info("Performance tester setup complete")

    def cleanup(self):
        """Clean up test environment"""
        if self.controller:
            if self.controller.is_high_frequency_control_running():
                self.controller.stop_high_frequency_control()

        logger.info("Performance tester cleanup complete")

    def run_all_tests(self) -> List[PerformanceMetric]:
        """Run all performance tests"""
        logger.info("Starting comprehensive performance testing")

        test_methods = [
            self.test_state_reading_performance,
            self.test_control_command_performance,
            self.test_high_frequency_control_performance,
            self.test_concurrent_operations,
            self.test_memory_allocation_performance,
            self.test_thread_safety_performance,
            self.test_latency_distribution,
            self.test_scalability_performance
        ]

        for test_method in test_methods:
            try:
                logger.info(f"Running {test_method.__name__}")
                results = test_method()
                self.results.extend(results)
                logger.info(f"Completed {test_method.__name__}")
            except Exception as e:
                logger.error(f"Error in {test_method.__name__}: {e}")
                logger.error(traceback.format_exc())

        return self.results

    def test_state_reading_performance(self) -> List[PerformanceMetric]:
        """Test state reading performance"""
        logger.info("Testing state reading performance...")

        test_cases = [
            {"name": "Low Frequency Reading", "target_freq": 50, "duration": 2.0},
            {"name": "Medium Frequency Reading", "target_freq": 200, "duration": 2.0},
            {"name": "High Frequency Reading", "target_freq": 500, "duration": 2.0}
        ]

        results = []

        for case in test_cases:
            self.monitor.start_monitoring()

            target_freq = case["target_freq"]
            duration = case["duration"]
            target_period = 1.0 / target_freq

            latencies = []
            start_time = time.time()
            operations = 0
            errors = 0

            while time.time() - start_time < duration:
                op_start = time.time()

                try:
                    # Read all states
                    positions = self.controller.get_joint_positions()
                    velocities = self.controller.get_joint_velocities()
                    torques = self.controller.get_joint_torques()

                    # Use the data to prevent optimization
                    _ = sum(positions) + sum(velocities) + sum(torques)
                    operations += 1

                except Exception as e:
                    errors += 1
                    logger.warning(f"State reading error: {e}")

                op_end = time.time()
                latencies.append((op_end - op_start) * 1000)  # Convert to ms

                # Rate limiting
                elapsed = op_end - op_start
                if elapsed < target_period:
                    time.sleep(target_period - elapsed)

            total_time = time.time() - start_time
            stats = self.monitor.get_current_stats()

            # Calculate metrics
            actual_frequency = operations / total_time
            avg_latency = statistics.mean(latencies)
            min_latency = min(latencies)
            max_latency = max(latencies)
            std_latency = statistics.stdev(latencies) if len(latencies) > 1 else 0
            success_rate = (operations / (operations + errors)) * 100 if (operations + errors) > 0 else 0

            metric = PerformanceMetric(
                test_name=case["name"],
                operation="state_reading",
                target_frequency=target_freq,
                actual_frequency=actual_frequency,
                avg_latency_ms=avg_latency,
                min_latency_ms=min_latency,
                max_latency_ms=max_latency,
                std_latency_ms=std_latency,
                success_rate=success_rate,
                cpu_usage_percent=stats['cpu_usage_percent'],
                memory_usage_mb=stats['memory_usage_mb'],
                total_operations=operations,
                duration_seconds=total_time,
                timestamp=time.strftime("%Y-%m-%d %H:%M:%S")
            )

            results.append(metric)
            logger.info(f"State reading ({case['name']}): {actual_frequency:.1f} Hz, {avg_latency:.3f} ms avg latency")

        return results

    def test_control_command_performance(self) -> List[PerformanceMetric]:
        """Test control command performance"""
        logger.info("Testing control command performance...")

        test_cases = [
            {"name": "Position Control", "command": "position", "target_freq": 100, "duration": 2.0},
            {"name": "Torque Control", "command": "torque", "target_freq": 100, "duration": 2.0},
            {"name": "Mixed Control", "command": "mixed", "target_freq": 100, "duration": 2.0}
        ]

        results = []

        for case in test_cases:
            self.monitor.start_monitoring()

            command_type = case["command"]
            target_freq = case["target_freq"]
            duration = case["duration"]
            target_period = 1.0 / target_freq

            latencies = []
            start_time = time.time()
            operations = 0
            errors = 0

            while time.time() - start_time < duration:
                op_start = time.time()

                try:
                    if command_type == "position":
                        positions = [0.1 * (operations % 10)] * 7
                        self.controller.set_joint_positions(positions)
                    elif command_type == "torque":
                        torques = [0.1] * 7
                        self.controller.set_joint_torques(torques)
                    else:  # mixed
                        if operations % 2 == 0:
                            positions = [0.1 * (operations % 10)] * 7
                            self.controller.set_joint_positions(positions)
                        else:
                            torques = [0.1] * 7
                            self.controller.set_joint_torques(torques)

                    operations += 1

                except Exception as e:
                    errors += 1
                    logger.warning(f"Control command error: {e}")

                op_end = time.time()
                latencies.append((op_end - op_start) * 1000)

                # Rate limiting
                elapsed = op_end - op_start
                if elapsed < target_period:
                    time.sleep(target_period - elapsed)

            total_time = time.time() - start_time
            stats = self.monitor.get_current_stats()

            # Calculate metrics
            actual_frequency = operations / total_time
            avg_latency = statistics.mean(latencies)
            min_latency = min(latencies)
            max_latency = max(latencies)
            std_latency = statistics.stdev(latencies) if len(latencies) > 1 else 0
            success_rate = (operations / (operations + errors)) * 100 if (operations + errors) > 0 else 0

            metric = PerformanceMetric(
                test_name=case["name"],
                operation="control_command",
                target_frequency=target_freq,
                actual_frequency=actual_frequency,
                avg_latency_ms=avg_latency,
                min_latency_ms=min_latency,
                max_latency_ms=max_latency,
                std_latency_ms=std_latency,
                success_rate=success_rate,
                cpu_usage_percent=stats['cpu_usage_percent'],
                memory_usage_mb=stats['memory_usage_mb'],
                total_operations=operations,
                duration_seconds=total_time,
                timestamp=time.strftime("%Y-%m-%d %H:%M:%S")
            )

            results.append(metric)
            logger.info(f"Control command ({case['name']}): {actual_frequency:.1f} Hz, {avg_latency:.3f} ms avg latency")

        return results

    def test_high_frequency_control_performance(self) -> List[PerformanceMetric]:
        """Test high-frequency control performance"""
        logger.info("Testing high-frequency control performance...")

        if not NATIVE_AVAILABLE:
            logger.info("Skipping high-frequency control test (native module not available)")
            return []

        test_frequencies = [100, 200, 500]  # Hz
        results = []

        for target_freq in test_frequencies:
            self.monitor.start_monitoring()

            # Set up high-frequency control
            self.controller.set_control_frequency(target_freq)
            self.controller.start_high_frequency_control()

            # Monitor for test duration
            test_duration = 3.0
            start_time = time.time()
            update_count = 0
            errors = 0

            while time.time() - start_time < test_duration:
                try:
                    self.controller.update_all_states()
                    update_count += 1
                    time.sleep(0.001)  # Small delay
                except Exception as e:
                    errors += 1
                    logger.warning(f"High-frequency control error: {e}")

            # Stop high-frequency control
            self.controller.stop_high_frequency_control()

            total_time = time.time() - start_time
            stats = self.monitor.get_current_stats()

            actual_frequency = update_count / total_time
            success_rate = (update_count / (update_count + errors)) * 100 if (update_count + errors) > 0 else 0

            metric = PerformanceMetric(
                test_name=f"High-Freq Control {target_freq}Hz",
                operation="high_frequency_control",
                target_frequency=target_freq,
                actual_frequency=actual_frequency,
                avg_latency_ms=1000.0 / actual_frequency if actual_frequency > 0 else 0,
                min_latency_ms=1000.0 / actual_frequency if actual_frequency > 0 else 0,
                max_latency_ms=1000.0 / actual_frequency if actual_frequency > 0 else 0,
                std_latency_ms=0,
                success_rate=success_rate,
                cpu_usage_percent=stats['cpu_usage_percent'],
                memory_usage_mb=stats['memory_usage_mb'],
                total_operations=update_count,
                duration_seconds=total_time,
                timestamp=time.strftime("%Y-%m-%d %H:%M:%S")
            )

            results.append(metric)
            logger.info(f"High-frequency control ({target_freq}Hz): {actual_frequency:.1f} Hz actual")

        return results

    def test_concurrent_operations(self) -> List[PerformanceMetric]:
        """Test concurrent operations performance"""
        logger.info("Testing concurrent operations performance...")

        thread_counts = [1, 2, 4, 8]
        operations_per_thread = 100
        results = []

        for num_threads in thread_counts:
            self.monitor.start_monitoring()

            start_time = time.time()
            total_operations = 0
            total_errors = 0

            def worker_function():
                nonlocal total_operations, total_errors
                local_ops = 0
                local_errors = 0

                for i in range(operations_per_thread):
                    try:
                        # Mix of operations
                        if i % 3 == 0:
                            positions = self.controller.get_joint_positions()
                        elif i % 3 == 1:
                            test_positions = [0.1 * i] * 7
                            self.controller.set_joint_positions(test_positions)
                        else:
                            self.controller.update_all_states()

                        local_ops += 1
                    except Exception as e:
                        local_errors += 1

                total_operations += local_ops
                total_errors += local_errors

            # Run threads
            with ThreadPoolExecutor(max_workers=num_threads) as executor:
                futures = [executor.submit(worker_function) for _ in range(num_threads)]
                for future in futures:
                    future.result()

            total_time = time.time() - start_time
            stats = self.monitor.get_current_stats()

            actual_frequency = total_operations / total_time
            success_rate = (total_operations / (total_operations + total_errors)) * 100 if (total_operations + total_errors) > 0 else 0

            metric = PerformanceMetric(
                test_name=f"Concurrent Operations {num_threads} Threads",
                operation="concurrent_operations",
                target_frequency=num_threads * operations_per_thread / total_time,
                actual_frequency=actual_frequency,
                avg_latency_ms=total_time / total_operations * 1000 if total_operations > 0 else 0,
                min_latency_ms=0,
                max_latency_ms=0,
                std_latency_ms=0,
                success_rate=success_rate,
                cpu_usage_percent=stats['cpu_usage_percent'],
                memory_usage_mb=stats['memory_usage_mb'],
                total_operations=total_operations,
                duration_seconds=total_time,
                timestamp=time.strftime("%Y-%m-%d %H:%M:%S")
            )

            results.append(metric)
            logger.info(f"Concurrent operations ({num_threads} threads): {actual_frequency:.1f} ops/s")

        return results

    def test_memory_allocation_performance(self) -> List[PerformanceMetric]:
        """Test memory allocation performance"""
        logger.info("Testing memory allocation performance...")

        allocation_sizes = [100, 1000, 10000]  # Number of operations
        results = []

        for size in allocation_sizes:
            self.monitor.start_monitoring()

            start_time = time.time()

            # Perform memory-intensive operations
            for i in range(size):
                # Allocate temporary data structures
                positions = self.controller.get_joint_positions()
                temp_data = {
                    'positions': positions,
                    'velocities': self.controller.get_joint_velocities(),
                    'torques': self.controller.get_joint_torques(),
                    'iteration': i,
                    'extra_data': list(range(100))
                }

                # Use the data
                _ = sum(temp_data['positions']) + sum(temp_data['velocities'])

                # Cleanup
                del temp_data

            total_time = time.time() - start_time
            stats = self.monitor.get_current_stats()

            actual_frequency = size / total_time

            metric = PerformanceMetric(
                test_name=f"Memory Allocation {size} Ops",
                operation="memory_allocation",
                target_frequency=0,
                actual_frequency=actual_frequency,
                avg_latency_ms=total_time / size * 1000,
                min_latency_ms=0,
                max_latency_ms=0,
                std_latency_ms=0,
                success_rate=100.0,
                cpu_usage_percent=stats['cpu_usage_percent'],
                memory_usage_mb=stats['memory_usage_mb'],
                total_operations=size,
                duration_seconds=total_time,
                timestamp=time.strftime("%Y-%m-%d %H:%M:%S")
            )

            results.append(metric)
            logger.info(f"Memory allocation ({size} ops): {actual_frequency:.1f} ops/s, {stats['memory_increase_mb']:.1f} MB increase")

        return results

    def test_thread_safety_performance(self) -> List[PerformanceMetric]:
        """Test thread safety performance overhead"""
        logger.info("Testing thread safety performance...")

        test_durations = [1.0, 2.0, 5.0]  # seconds
        results = []

        for duration in test_durations:
            self.monitor.start_monitoring()

            # Single-threaded baseline
            single_start = time.time()
            single_ops = 0
            while time.time() - single_start < duration:
                positions = self.controller.get_joint_positions()
                single_ops += 1
            single_time = time.time() - single_start

            # Multi-threaded test
            multi_start = time.time()
            multi_ops = 0

            def worker():
                nonlocal multi_ops
                end_time = time.time() + duration
                while time.time() < end_time:
                    positions = self.controller.get_joint_positions()
                    multi_ops += 1

            threads = []
            for _ in range(4):
                thread = threading.Thread(target=worker)
                threads.append(thread)
                thread.start()

            for thread in threads:
                thread.join()

            multi_time = time.time() - multi_start
            stats = self.monitor.get_current_stats()

            # Calculate overhead
            overhead = ((multi_time - single_time) / single_time) * 100 if single_time > 0 else 0

            metric = PerformanceMetric(
                test_name=f"Thread Safety {duration}s",
                operation="thread_safety",
                target_frequency=0,
                actual_frequency=multi_ops / multi_time,
                avg_latency_ms=multi_time / multi_ops * 1000 if multi_ops > 0 else 0,
                min_latency_ms=0,
                max_latency_ms=0,
                std_latency_ms=0,
                success_rate=100.0,
                cpu_usage_percent=stats['cpu_usage_percent'],
                memory_usage_mb=stats['memory_usage_mb'],
                total_operations=multi_ops,
                duration_seconds=multi_time,
                timestamp=time.strftime("%Y-%m-%d %H:%M:%S")
            )

            results.append(metric)
            logger.info(f"Thread safety ({duration}s): {overhead:.1f}% overhead, {multi_ops/multi_time:.1f} ops/s")

        return results

    def test_latency_distribution(self) -> List[PerformanceMetric]:
        """Test latency distribution in detail"""
        logger.info("Testing latency distribution...")

        self.monitor.start_monitoring()

        num_samples = 1000
        latencies = []

        for i in range(num_samples):
            start_time = time.time()

            # Perform operation
            positions = self.controller.get_joint_positions()

            end_time = time.time()
            latencies.append((end_time - start_time) * 1000)  # ms

            # Small delay
            time.sleep(0.001)

        stats = self.monitor.get_current_stats()

        # Calculate detailed statistics
        percentiles = {
            'p50': np.percentile(latencies, 50),
            'p90': np.percentile(latencies, 90),
            'p95': np.percentile(latencies, 95),
            'p99': np.percentile(latencies, 99)
        }

        metric = PerformanceMetric(
            test_name="Latency Distribution",
            operation="latency_analysis",
            target_frequency=0,
            actual_frequency=num_samples / (sum(latencies) / 1000),
            avg_latency_ms=statistics.mean(latencies),
            min_latency_ms=min(latencies),
            max_latency_ms=max(latencies),
            std_latency_ms=statistics.stdev(latencies),
            success_rate=100.0,
            cpu_usage_percent=stats['cpu_usage_percent'],
            memory_usage_mb=stats['memory_usage_mb'],
            total_operations=num_samples,
            duration_seconds=sum(latencies) / 1000,
            timestamp=time.strftime("%Y-%m-%d %H:%M:%S")
        )

        logger.info(f"Latency distribution - Mean: {metric.avg_latency_ms:.3f}ms, "
                   f"P50: {percentiles['p50']:.3f}ms, P95: {percentiles['p95']:.3f}ms, "
                   f"P99: {percentiles['p99']:.3f}ms")

        return [metric]

    def test_scalability_performance(self) -> List[PerformanceMetric]:
        """Test scalability with different operation intensities"""
        logger.info("Testing scalability performance...")

        intensities = [
            {"name": "Light Load", "ops_per_sec": 50, "duration": 3.0},
            {"name": "Medium Load", "ops_per_sec": 200, "duration": 3.0},
            {"name": "Heavy Load", "ops_per_sec": 500, "duration": 3.0}
        ]

        results = []

        for intensity in intensities:
            self.monitor.start_monitoring()

            target_ops_per_sec = intensity["ops_per_sec"]
            duration = intensity["duration"]
            target_period = 1.0 / target_ops_per_sec

            start_time = time.time()
            operations = 0
            latencies = []

            while time.time() - start_time < duration:
                op_start = time.time()

                # Perform mixed operations
                if operations % 3 == 0:
                    positions = self.controller.get_joint_positions()
                elif operations % 3 == 1:
                    test_positions = [0.1 * (operations % 10)] * 7
                    self.controller.set_joint_positions(test_positions)
                else:
                    self.controller.update_all_states()

                op_end = time.time()
                latencies.append((op_end - op_start) * 1000)
                operations += 1

                # Rate limiting
                elapsed = op_end - op_start
                if elapsed < target_period:
                    time.sleep(target_period - elapsed)

            total_time = time.time() - start_time
            stats = self.monitor.get_current_stats()

            actual_frequency = operations / total_time
            avg_latency = statistics.mean(latencies)

            metric = PerformanceMetric(
                test_name=f"Scalability {intensity['name']}",
                operation="scalability",
                target_frequency=target_ops_per_sec,
                actual_frequency=actual_frequency,
                avg_latency_ms=avg_latency,
                min_latency_ms=min(latencies),
                max_latency_ms=max(latencies),
                std_latency_ms=statistics.stdev(latencies) if len(latencies) > 1 else 0,
                success_rate=100.0,
                cpu_usage_percent=stats['cpu_usage_percent'],
                memory_usage_mb=stats['memory_usage_mb'],
                total_operations=operations,
                duration_seconds=total_time,
                timestamp=time.strftime("%Y-%m-%d %H:%M:%S")
            )

            results.append(metric)
            logger.info(f"Scalability ({intensity['name']}): {actual_frequency:.1f} ops/s, {avg_latency:.3f} ms avg latency")

        return results

    def save_results(self, filename: str = "performance_results.csv"):
        """Save performance results to CSV file"""
        if not self.results:
            logger.warning("No results to save")
            return

        with open(filename, 'w', newline='') as csvfile:
            fieldnames = asdict(self.results[0]).keys()
            writer = csv.DictWriter(csvfile, fieldnames=fieldnames)

            writer.writeheader()
            for result in self.results:
                writer.writerow(asdict(result))

        logger.info(f"Results saved to {filename}")

    def generate_report(self, filename: str = "performance_report.json"):
        """Generate comprehensive performance report"""
        if not self.results:
            logger.warning("No results to report")
            return

        # Calculate summary statistics
        summary = {
            'total_tests': len(self.results),
            'timestamp': time.strftime("%Y-%m-%d %H:%M:%S"),
            'native_available': NATIVE_AVAILABLE,
            'device_sn': self.device_sn
        }

        # Group results by operation type
        by_operation = {}
        for result in self.results:
            op_type = result.operation
            if op_type not in by_operation:
                by_operation[op_type] = []
            by_operation[op_type].append(result)

        # Calculate statistics for each operation type
        operation_stats = {}
        for op_type, results in by_operation.items():
            frequencies = [r.actual_frequency for r in results]
            latencies = [r.avg_latency_ms for r in results]

            operation_stats[op_type] = {
                'count': len(results),
                'avg_frequency': statistics.mean(frequencies),
                'max_frequency': max(frequencies),
                'min_frequency': min(frequencies),
                'avg_latency': statistics.mean(latencies),
                'min_latency': min(latencies),
                'max_latency': max(latencies)
            }

        # Full report
        report = {
            'summary': summary,
            'operation_stats': operation_stats,
            'detailed_results': [asdict(r) for r in self.results]
        }

        with open(filename, 'w') as f:
            json.dump(report, f, indent=2)

        logger.info(f"Performance report saved to {filename}")

    def plot_results(self, output_dir: str = "plots"):
        """Generate performance plots"""
        if not self.results:
            logger.warning("No results to plot")
            return

        try:
            import matplotlib.pyplot as plt
            import numpy as np

            os.makedirs(output_dir, exist_ok=True)

            # Plot 1: Frequency vs Operation Type
            plt.figure(figsize=(12, 6))
            operations = list(set(r.operation for r in self.results))
            frequencies_by_op = {op: [] for op in operations}

            for result in self.results:
                frequencies_by_op[result.operation].append(result.actual_frequency)

            x = np.arange(len(operations))
            means = [statistics.mean(frequencies_by_op[op]) for op in operations]
            stds = [statistics.stdev(frequencies_by_op[op]) if len(frequencies_by_op[op]) > 1 else 0 for op in operations]

            plt.bar(x, means, yerr=stds, capsize=5)
            plt.xlabel('Operation Type')
            plt.ylabel('Frequency (Hz)')
            plt.title('Performance by Operation Type')
            plt.xticks(x, operations, rotation=45)
            plt.tight_layout()
            plt.savefig(f"{output_dir}/frequency_by_operation.png")
            plt.close()

            # Plot 2: Latency Distribution
            plt.figure(figsize=(10, 6))
            all_latencies = [r.avg_latency_ms for r in self.results]
            plt.hist(all_latencies, bins=20, alpha=0.7)
            plt.xlabel('Average Latency (ms)')
            plt.ylabel('Count')
            plt.title('Latency Distribution')
            plt.savefig(f"{output_dir}/latency_distribution.png")
            plt.close()

            logger.info(f"Performance plots saved to {output_dir}/")

        except ImportError:
            logger.warning("Matplotlib not available, skipping plot generation")


def main():
    """Main performance testing function"""
    logger.info("Starting IC_CAN Python Performance Testing")

    # Create tester
    tester = IC_CANPerformanceTester()

    try:
        # Set up test environment
        tester.setup()

        # Run all tests
        results = tester.run_all_tests()

        # Save results
        tester.save_results()
        tester.generate_report()
        tester.plot_results()

        # Print summary
        logger.info("=" * 60)
        logger.info("PERFORMANCE TEST SUMMARY")
        logger.info("=" * 60)
        logger.info(f"Total tests performed: {len(results)}")

        if results:
            frequencies = [r.actual_frequency for r in results]
            latencies = [r.avg_latency_ms for r in results]

            logger.info(f"Average frequency: {statistics.mean(frequencies):.1f} Hz")
            logger.info(f"Max frequency: {max(frequencies):.1f} Hz")
            logger.info(f"Average latency: {statistics.mean(latencies):.3f} ms")
            logger.info(f"Min latency: {min(latencies):.3f} ms")
            logger.info(f"Max latency: {max(latencies):.3f} ms")

        logger.info("=" * 60)
        logger.info("Results saved to:")
        logger.info("  - performance_results.csv")
        logger.info("  - performance_report.json")
        logger.info("  - plots/ (directory)")
        logger.info("=" * 60)

        return True

    except Exception as e:
        logger.error(f"Performance testing failed: {e}")
        logger.error(traceback.format_exc())
        return False

    finally:
        tester.cleanup()


if __name__ == "__main__":
    success = main()
    sys.exit(0 if success else 1)