#!/usr/bin/env python3
"""
Friction Calibration Tool for IC_CAN System

Uses logged motor data from trajectory execution to identify friction parameters:
- Coulomb friction (Fc) - static friction coefficient
- Viscous friction (Fv) - velocity-dependent friction coefficient
- Asymmetric friction - different values for positive/negative directions
- Velocity threshold - minimum velocity for friction compensation

Usage:
    python3 friction_calibration.py <motor_states.csv>
"""

import argparse
import csv
import json
import numpy as np
import matplotlib.pyplot as plt
from pathlib import Path


class FrictionCalibrator:
    def __init__(self):
        self.motors_data = {}
        self.friction_params = {}

    def load_motor_data(self, csv_file):
        """Load motor state data from CSV log file"""
        print(f"📂 Loading motor data from: {csv_file}")

        try:
            with open(csv_file, 'r') as f:
                reader = csv.reader(f)
                header = next(reader)

                # Initialize motor data storage
                for motor_idx in range(1, 10):  # Motors 1-9
                    self.motors_data[motor_idx] = {
                        'timestamps': [],
                        'positions': [],
                        'velocities': [],
                        'torques': []
                    }

                # Parse CSV rows
                for row in reader:
                    timestamp = row[0]
                    base_col = 1  # Starting column for motor 1

                    for motor_idx in range(1, 10):
                        pos_col = base_col + (motor_idx - 1) * 3
                        vel_col = pos_col + 1
                        torque_col = pos_col + 2

                        if len(row) > torque_col:
                            self.motors_data[motor_idx]['timestamps'].append(timestamp)
                            self.motors_data[motor_idx]['positions'].append(float(row[pos_col]) if row[pos_col] else 0.0)
                            self.motors_data[motor_idx]['velocities'].append(float(row[vel_col]) if row[vel_col] else 0.0)
                            self.motors_data[motor_idx]['torques'].append(float(row[torque_col]) if row[torque_col] else 0.0)

                print(f"✅ Loaded data for {len(self.motors_data)} motors")

        except Exception as e:
            print(f"❌ Error loading motor data: {e}")
            return False

        return True

    def identify_friction_parameters(self, motor_idx):
        """Identify friction parameters for a specific motor"""
        data = self.motors_data[motor_idx]
        velocities = np.array(data['velocities'])
        torques = np.array(data['torques'])

        # Check if motor was active
        if np.max(np.abs(velocities)) < 0.01:
            print(f"🔴 Motor {motor_idx}: INACTIVE (no motion detected)")
            return None

        print(f"🔍 Analyzing Motor {motor_idx} friction...")

        # Identify static friction (near-zero velocity)
        low_vel_threshold = 0.1  # rad/s
        low_vel_mask = np.abs(velocities) < low_vel_threshold

        # Positive direction static friction
        pos_mask = (velocities > 0) & low_vel_mask
        neg_mask = (velocities < 0) & low_vel_mask

        static_friction_pos = 0
        static_friction_neg = 0

        if np.sum(pos_mask) > 0:
            static_friction_pos = np.mean(torques[pos_mask])
        if np.sum(neg_mask) > 0:
            static_friction_neg = np.mean(torques[neg_mask])

        # Identify dynamic friction (higher velocity)
        high_vel_threshold = 0.2  # rad/s
        high_vel_mask = np.abs(velocities) > high_vel_threshold

        dynamic_friction_pos = 0
        dynamic_friction_neg = 0

        if np.sum((velocities > high_vel_threshold)) > 0:
            dynamic_friction_pos = np.mean(torques[velocities > high_vel_threshold])
        if np.sum((velocities < -high_vel_threshold)) > 0:
            dynamic_friction_neg = np.mean(torques[velocities < -high_vel_threshold])

        # Calculate friction parameters
        coulomb_friction = (abs(static_friction_pos) + abs(static_friction_neg)) / 2

        # Estimate viscous friction coefficient
        avg_pos_vel = np.mean(velocities[velocities > high_vel_threshold]) if np.sum(velocities > high_vel_threshold) > 0 else 0
        avg_neg_vel = np.mean(velocities[velocities < -high_vel_threshold]) if np.sum(velocities < -high_vel_threshold) > 0 else 0

        viscous_coeff_pos = (dynamic_friction_pos - static_friction_pos) / avg_pos_vel if avg_pos_vel != 0 else 0
        viscous_coeff_neg = (dynamic_friction_neg - static_friction_neg) / avg_neg_vel if avg_neg_vel != 0 else 0

        viscous_friction = (abs(viscous_coeff_pos) + abs(viscous_coeff_neg)) / 2

        # Set velocity threshold based on data
        velocity_threshold = max(low_vel_threshold, np.std(velocities[low_vel_mask]) * 2)

        # Store identified parameters
        friction_params = {
            'joint': motor_idx - 1,  # 0-indexed for JSON config
            'Fc': coulomb_friction,
            'Fv': viscous_friction,
            'Fc_positive': static_friction_pos,
            'Fc_negative': static_friction_neg,
            'Fv_positive': viscous_coeff_pos,
            'Fv_negative': viscous_coeff_neg,
            'velocity_threshold': velocity_threshold,
            'data_points': len(velocities),
            'active_points': int(np.sum(np.abs(velocities) > 0.01)),
            'max_velocity': float(np.max(np.abs(velocities))),
            'max_torque': float(np.max(np.abs(torques)))
        }

        self.friction_params[motor_idx] = friction_params

        # Print analysis results
        self.print_motor_analysis(motor_idx, friction_params, velocities, torques)

        return friction_params

    def print_motor_analysis(self, motor_idx, params, velocities, torques):
        """Print detailed friction analysis for a motor"""
        print(f"  📊 Motor {motor_idx} Friction Analysis:")
        print(f"     Coulomb Friction (Fc): {params['Fc']:.3f}")
        print(f"     Static Fric (+): {params['Fc_positive']:.3f}")
        print(f"     Static Fric (-): {params['Fc_negative']:.3f}")
        print(f"     Viscous Coeff (Fv): {params['Fv']:.3f}")
        print(f"     Velocity Threshold: {params['velocity_threshold']:.3f} rad/s")
        print(f"     Active Data Points: {params['active_points']}/{params['data_points']}")
        print(f"     Max Velocity: {params['max_velocity']:.3f} rad/s")
        print(f"     Max Torque: {params['max_torque']:.3f}")

    def analyze_all_motors(self):
        """Analyze friction for all motors"""
        print("\n🔍 STARTING FRICTION CALIBRATION")
        print("=" * 60)

        for motor_idx in range(1, 10):
            self.identify_friction_parameters(motor_idx)

        self.print_calibration_summary()

    def print_calibration_summary(self):
        """Print summary of all calibrated friction parameters"""
        print("\n📈 CALIBRATION SUMMARY")
        print("=" * 80)
        print(f"{'Motor':<8} {'Fc':<8} {'Fv':<8} {'Vel Thresh':<12} {'Active':<8} {'Max Vel':<10}")
        print("-" * 80)

        for motor_idx in sorted(self.friction_params.keys()):
            if motor_idx <= 6:  # Only show active motors (1-6)
                params = self.friction_params[motor_idx]
                print(f"{motor_idx:<8} {params['Fc']:<8.3f} {params['Fv']:<8.3f} {params['velocity_threshold']:<12.3f} "
                      f"{params['active_points']:<8} {params['max_velocity']:<10.3f}")

        print("-" * 80)

    def generate_friction_config(self, output_file="friction_calibrated_params.json"):
        """Generate JSON configuration file with calibrated friction parameters"""
        print(f"\n💾 Generating calibrated friction config: {output_file}")

        # Build friction parameters array for active motors (1-6)
        friction_config = {
            "calibration_timestamp": str(Path(__file__).stat().st_mtime),
            "calibration_method": "experimental_trajectory_analysis",
            "enable_friction": True,
            "enable_gravity": True,
            "velocity_damping": 0.1,
            "smooth_transition": True,
            "sgn_threshold": 0.01,
            "friction_params": []
        }

        for motor_idx in range(1, 7):  # Motors 1-6 for trajectory
            if motor_idx in self.friction_params:
                params = self.friction_params[motor_idx]
                friction_config["friction_params"].append({
                    "joint": params['joint'],
                    "Fc": round(params['Fc'], 3),
                    "Fv": round(params['Fv'], 3),
                    "Fc_positive": round(params['Fc_positive'], 3),
                    "Fc_negative": round(params['Fc_negative'], 3),
                    "velocity_threshold": round(params['velocity_threshold'], 3),
                    "calibration_data_points": int(params['data_points']),
                    "calibration_active_points": int(params['active_points'])
                })
            else:
                # Default parameters for inactive motors
                friction_config["friction_params"].append({
                    "joint": motor_idx - 1,
                    "Fc": 0.1,
                    "Fv": 0.01,
                    "velocity_threshold": 0.05,
                    "note": "motor_inactive_during_calibration"
                })

        # Save to JSON file
        with open(output_file, 'w') as f:
            json.dump(friction_config, f, indent=2)

        print(f"✅ Calibrated friction parameters saved to: {output_file}")

        # Also generate comparison with original parameters
        self.generate_comparison_report(output_file)

        return output_file

    def generate_comparison_report(self, calibrated_file):
        """Generate comparison with original friction parameters"""
        original_file = "friction_trajectory_friction_params.json"
        comparison_file = calibrated_file.replace('.json', '_comparison.json')

        # Try to load original parameters
        original_params = {}
        try:
            with open(original_file, 'r') as f:
                orig_data = json.load(f)
                for param in orig_data.get('friction_params', []):
                    original_params[param['joint']] = param
        except:
            print(f"⚠️  Could not load original parameters from {original_file}")

        # Generate comparison
        comparison = {
            "calibration_comparison": {
                "original_file": original_file,
                "calibrated_file": calibrated_file,
                "motors_comparison": []
            }
        }

        for motor_idx in range(1, 7):
            joint_idx = motor_idx - 1
            if joint_idx in self.friction_params:
                calibrated = self.friction_params[motor_idx]
                original = original_params.get(joint_idx, {})

                comparison["calibration_comparison"]["motors_comparison"].append({
                    "joint": joint_idx,
                    "motor": motor_idx,
                    "original_Fc": original.get('Fc', 'N/A'),
                    "calibrated_Fc": round(calibrated['Fc'], 3),
                    "original_Fv": original.get('Fv', 'N/A'),
                    "calibrated_Fv": round(calibrated['Fv'], 3),
                    "Fc_change": self.calculate_percentage_change(
                        original.get('Fc', calibrated['Fc']), calibrated['Fc']),
                    "Fv_change": self.calculate_percentage_change(
                        original.get('Fv', calibrated['Fv']), calibrated['Fv'])
                })

        with open(comparison_file, 'w') as f:
            json.dump(comparison, f, indent=2)

        print(f"📊 Comparison report saved to: {comparison_file}")

    def calculate_percentage_change(self, original, new):
        """Calculate percentage change between original and new values"""
        if original == 'N/A' or original == 0:
            return 'N/A'
        change = ((new - original) / original) * 100
        return f"{change:+.1f}%"

    def plot_friction_analysis(self, output_dir="friction_plots"):
        """Generate friction analysis plots"""
        try:
            import os
            os.makedirs(output_dir, exist_ok=True)

            print(f"\n📈 Generating friction plots in: {output_dir}")

            # Create subplot for each active motor
            active_motors = [m for m in range(1, 7) if m in self.friction_params]

            if not active_motors:
                print("⚠️  No active motors found for plotting")
                return

            fig, axes = plt.subplots(2, 3, figsize=(15, 10))
            fig.suptitle('Friction Analysis - Torque vs Velocity', fontsize=16)

            for i, motor_idx in enumerate(active_motors[:6]):
                row, col = i // 3, i % 3
                ax = axes[row, col]

                data = self.motors_data[motor_idx]
                velocities = np.array(data['velocities'])
                torques = np.array(data['torques'])

                # Filter out zero velocity for better visualization
                active_mask = np.abs(velocities) > 0.01
                v_active = velocities[active_mask]
                t_active = torques[active_mask]

                # Plot
                ax.scatter(v_active, t_active, alpha=0.6, s=1)
                ax.set_xlabel('Velocity (rad/s)')
                ax.set_ylabel('Torque')
                ax.set_title(f'Motor {motor_idx} (Fc={self.friction_params[motor_idx]["Fc"]:.2f})')
                ax.grid(True, alpha=0.3)

                # Add zero lines
                ax.axhline(y=0, color='k', linestyle='-', alpha=0.3)
                ax.axvline(x=0, color='k', linestyle='-', alpha=0.3)

            plt.tight_layout()
            plot_file = f"{output_dir}/friction_analysis.png"
            plt.savefig(plot_file, dpi=150, bbox_inches='tight')
            plt.close()

            print(f"✅ Friction plots saved to: {plot_file}")

        except ImportError:
            print("⚠️  Matplotlib not available - skipping plots")
        except Exception as e:
            print(f"❌ Error generating plots: {e}")


def main():
    parser = argparse.ArgumentParser(description='Friction Calibration for IC_CAN System')
    parser.add_argument('csv_file', help='Motor states CSV file from trajectory execution')
    parser.add_argument('-o', '--output', default='friction_calibrated_params.json',
                       help='Output friction parameters file')
    parser.add_argument('-p', '--plots', action='store_true',
                       help='Generate friction analysis plots')
    parser.add_argument('--plot-dir', default='friction_plots',
                       help='Directory to save plots (default: friction_plots)')

    args = parser.parse_args()

    # Check if CSV file exists
    if not Path(args.csv_file).exists():
        print(f"❌ Error: CSV file not found: {args.csv_file}")
        return 1

    print("🔧 IC_CAN Friction Calibration Tool")
    print("=" * 50)

    # Create calibrator and run analysis
    calibrator = FrictionCalibrator()

    if not calibrator.load_motor_data(args.csv_file):
        return 1

    # Analyze friction for all motors
    calibrator.analyze_all_motors()

    # Generate calibrated friction configuration
    calibrator.generate_friction_config(args.output)

    # Generate plots if requested
    if args.plots:
        calibrator.plot_friction_analysis(args.plot_dir)

    print("\n🎉 Friction calibration completed!")
    print(f"📋 Use calibrated parameters: {args.output}")
    print("🚀 Test improved trajectory performance with new friction parameters!")

    return 0


if __name__ == "__main__":
    exit(main())