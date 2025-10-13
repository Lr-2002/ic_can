#!/bin/bash

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

set -e  # Exit on any error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
BUILD_DIR="build"
INSTALL_DIR="install"
LOG_FILE="build_test.log"

# Function to print colored output
print_status() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Function to log output
log_output() {
    echo "$1" | tee -a "$LOG_FILE"
}

# Clean previous build
clean_build() {
    print_status "Cleaning previous build..."
    if [ -d "$BUILD_DIR" ]; then
        rm -rf "$BUILD_DIR"
        log_output "Removed existing build directory"
    fi
    if [ -d "$INSTALL_DIR" ]; then
        rm -rf "$INSTALL_DIR"
        log_output "Removed existing install directory"
    fi
    print_success "Build directories cleaned"
}

# Check dependencies
check_dependencies() {
    print_status "Checking dependencies..."

    # Check for CMake
    if ! command -v cmake &> /dev/null; then
        print_error "CMake is not installed"
        exit 1
    fi

    # Check for Python
    if ! command -v python3 &> /dev/null; then
        print_error "Python 3 is not installed"
        exit 1
    fi

    # Check for Git (for submodules)
    if ! command -v git &> /dev/null; then
        print_error "Git is not installed"
        exit 1
    fi

    # Check for compilers
    if ! command -v g++ &> /dev/null && ! command -v clang++ &> /dev/null; then
        print_error "C++ compiler (g++ or clang++) is not installed"
        exit 1
    fi

    print_success "All required dependencies found"
}

# Initialize Git submodules (if any)
init_submodules() {
    print_status "Initializing Git submodules..."
    if [ -f ".gitmodules" ]; then
        git submodule update --init --recursive
        log_output "Git submodules initialized"
    else
        print_warning "No .gitmodules file found"
    fi
}

# Configure build
configure_build() {
    print_status "Configuring build..."

    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"

    # Configure CMake
    cmake_config=(
        -DCMAKE_BUILD_TYPE=Release
        -DCMAKE_INSTALL_PREFIX="../$INSTALL_DIR"
        -DBUILD_TESTS=ON
        -DBUILD_EXAMPLES=ON
        -DBUILD_PYTHON_BINDINGS=ON
        -DENABLE_PYTHON_DEBUG_SYMBOLS=OFF
        -DPYBIND11_DETAILED_ERROR_MESSAGES=ON
    )

    # Add debug mode if requested
    if [ "$DEBUG_BUILD" = "true" ]; then
        cmake_config+=(-DCMAKE_BUILD_TYPE=Debug -DDEVELOPMENT_MODE=ON)
        print_status "Configuring debug build"
    else
        print_status "Configuring release build"
    fi

    # Add coverage if requested
    if [ "$ENABLE_COVERAGE" = "true" ]; then
        cmake_config+=(-DENABLE_COVERAGE=ON)
        print_status "Enabling test coverage"
    fi

    # Add sanitizer if requested
    if [ "$ENABLE_SANITIZER" = "true" ]; then
        cmake_config+=(-DENABLE_SANITIZER=ON)
        print_status "Enabling address sanitizer"
    fi

    log_output "Running CMake configuration..."
    log_output "Command: cmake ${cmake_config[*]} .."

    if cmake "${cmake_config[@]}" .. 2>&1 | tee -a "../$LOG_FILE"; then
        print_success "CMake configuration successful"
    else
        print_error "CMake configuration failed"
        exit 1
    fi

    cd ..
}

# Build project
build_project() {
    print_status "Building project..."

    cd "$BUILD_DIR"

    # Determine number of build jobs
    if command -v nproc &> /dev/null; then
        BUILD_JOBS=$(nproc)
    elif command -v sysctl &> /dev/null; then
        BUILD_JOBS=$(sysctl -n hw.ncpu)
    else
        BUILD_JOBS=4
    fi

    print_status "Using $BUILD_JOBS build jobs"

    log_output "Building with make -j$BUILD_JOBS..."
    if make -j"$BUILD_JOBS" 2>&1 | tee -a "../$LOG_FILE"; then
        print_success "Build completed successfully"
    else
        print_error "Build failed"
        exit 1
    fi

    cd ..
}

# Run C++ tests
run_cpp_tests() {
    print_status "Running C++ tests..."

    cd "$BUILD_DIR"

    # Create test results directory
    mkdir -p test_results

    # Run tests
    log_output "Running C++ tests..."
    if ctest --output-on-failure --test-dir test_results 2>&1 | tee -a "../$LOG_FILE"; then
        print_success "All C++ tests passed"
        CPP_TESTS_PASSED=true
    else
        print_warning "Some C++ tests failed"
        CPP_TESTS_PASSED=false
    fi

    # Run individual test binaries for detailed output
    if [ -f "test_results/test_basic" ]; then
        print_status "Running basic functionality tests..."
        ./test_results/test_basic 2>&1 | tee -a "../$LOG_FILE"
    fi

    if [ -f "test_results/test_performance" ]; then
        print_status "Running performance tests..."
        ./test_results/test_performance 2>&1 | tee -a "../$LOG_FILE"
    fi

    cd ..
}

# Run Python tests
run_python_tests() {
    print_status "Running Python tests..."

    # Set Python path
    export PYTHONPATH="$BUILD_DIR/python:$INSTALL_DIR/lib/python3/site-packages:$PYTHONPATH"

    # Check if Python module is available
    if python3 -c "import ic_can_python" 2>/dev/null; then
        print_success "Python module found and importable"
    else
        print_warning "Python module not importable, tests may fail in simulation mode"
    fi

    # Run Python functionality tests
    if [ -f "python/test_functionality.py" ]; then
        print_status "Running Python functionality tests..."
        if python3 python/test_functionality.py 2>&1 | tee -a "$LOG_FILE"; then
            print_success "Python functionality tests passed"
            PYTHON_TESTS_PASSED=true
        else
            print_warning "Python functionality tests failed"
            PYTHON_TESTS_PASSED=false
        fi
    else
        print_warning "Python functionality test script not found"
        PYTHON_TESTS_PASSED=false
    fi

    # Run Python performance tests
    if [ -f "python/test_performance.py" ]; then
        print_status "Running Python performance tests..."
        if python3 python/test_performance.py 2>&1 | tee -a "$LOG_FILE"; then
            print_success "Python performance tests passed"
        else
            print_warning "Python performance tests failed"
        fi
    else
        print_warning "Python performance test script not found"
    fi
}

# Install project
install_project() {
    print_status "Installing project..."

    cd "$BUILD_DIR"

    if make install 2>&1 | tee -a "../$LOG_FILE"; then
        print_success "Installation completed"
    else
        print_warning "Installation failed"
    fi

    cd ..
}

# Generate documentation
generate_docs() {
    print_status "Generating documentation..."

    if command -v doxygen &> /dev/null; then
        if [ -f "Doxyfile" ]; then
            doxygen Doxyfile 2>&1 | tee -a "$LOG_FILE"
            print_success "Documentation generated"
        else
            print_warning "Doxyfile not found"
        fi
    else
        print_warning "Doxygen not installed, skipping documentation generation"
    fi
}

# Run performance benchmarks
run_benchmarks() {
    print_status "Running performance benchmarks..."

    cd "$BUILD_DIR"

    # Run benchmark examples if available
    if [ -f "examples/performance_benchmark" ]; then
        print_status "Running C++ performance benchmark..."
        ./examples/performance_benchmark 2>&1 | tee -a "../$LOG_FILE"
    fi

    cd ..
}

# Generate build report
generate_report() {
    print_status "Generating build report..."

    REPORT_FILE="build_report.txt"

    cat > "$REPORT_FILE" << EOF
IC_CAN Build and Test Report
============================
Build Date: $(date)
Git Commit: $(git rev-parse HEAD 2>/dev/null || echo "N/A")
Branch: $(git branch --show-current 2>/dev/null || echo "N/A")

Build Configuration:
- Build Type: ${DEBUG_BUILD:-Release}
- Tests Enabled: Yes
- Python Bindings: Yes
- Examples: Yes
- Coverage: ${ENABLE_COVERAGE:-No}
- Sanitizer: ${ENABLE_SANITIZER:-No}

Test Results:
- C++ Tests: ${CPP_TESTS_PASSED:-Unknown}
- Python Tests: ${PYTHON_TESTS_PASSED:-Unknown}

Build Artifacts:
- Build Directory: $BUILD_DIR
- Install Directory: $INSTALL_DIR
- Log File: $LOG_FILE

Performance Files:
- C++ Performance: performance_results.csv (if generated)
- Python Performance: performance_results.csv (if generated)
- Performance Report: performance_report.json (if generated)

Notes:
EOF

    # Add warnings from log
    if grep -i "warning" "$LOG_FILE" > /dev/null; then
        echo "Warnings found during build:" >> "$REPORT_FILE"
        grep -i "warning" "$LOG_FILE" | head -10 >> "$REPORT_FILE"
        echo "" >> "$REPORT_FILE"
    fi

    # Add errors from log
    if grep -i "error" "$LOG_FILE" > /dev/null; then
        echo "Errors found during build:" >> "$REPORT_FILE"
        grep -i "error" "$LOG_FILE" | head -10 >> "$REPORT_FILE"
    fi

    print_success "Build report generated: $REPORT_FILE"
}

# Main execution
main() {
    print_status "Starting IC_CAN build and test process..."

    # Initialize log file
    echo "IC_CAN Build and Test Log - $(date)" > "$LOG_FILE"
    echo "========================================" >> "$LOG_FILE"

    # Parse command line arguments
    while [[ $# -gt 0 ]]; do
        case $1 in
            --debug)
                DEBUG_BUILD=true
                shift
                ;;
            --coverage)
                ENABLE_COVERAGE=true
                shift
                ;;
            --sanitizer)
                ENABLE_SANITIZER=true
                shift
                ;;
            --clean)
                clean_build
                exit 0
                ;;
            --no-tests)
                SKIP_TESTS=true
                shift
                ;;
            --no-install)
                SKIP_INSTALL=true
                shift
                ;;
            --help)
                echo "Usage: $0 [OPTIONS]"
                echo "Options:"
                echo "  --debug       Enable debug build"
                echo "  --coverage    Enable test coverage"
                echo "  --sanitizer   Enable address sanitizer"
                echo "  --clean       Clean build directories only"
                echo "  --no-tests    Skip running tests"
                echo "  --no-install  Skip installation"
                echo "  --help        Show this help message"
                exit 0
                ;;
            *)
                print_error "Unknown option: $1"
                exit 1
                ;;
        esac
    done

    # Execute build steps
    check_dependencies
    init_submodules
    configure_build
    build_project

    if [ "$SKIP_TESTS" != "true" ]; then
        run_cpp_tests
        run_python_tests
        run_benchmarks
    else
        print_warning "Skipping tests as requested"
    fi

    if [ "$SKIP_INSTALL" != "true" ]; then
        install_project
    else
        print_warning "Skipping installation as requested"
    fi

    generate_docs
    generate_report

    print_success "Build and test process completed!"
    print_status "Check the following files for detailed results:"
    print_status "  - Build log: $LOG_FILE"
    print_status "  - Build report: build_report.txt"
    print_status "  - Performance results: performance_results.csv"
    print_status "  - Install directory: $INSTALL_DIR"

    # Return appropriate exit code
    if [ "$SKIP_TESTS" = "true" ]; then
        exit 0
    elif [ "$CPP_TESTS_PASSED" = "true" ] && [ "$PYTHON_TESTS_PASSED" = "true" ]; then
        exit 0
    else
        print_warning "Some tests failed, but build completed"
        exit 1
    fi
}

# Script entry point
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    main "$@"
fi