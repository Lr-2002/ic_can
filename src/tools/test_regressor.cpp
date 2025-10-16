#include <iostream>
#include <vector>
#include <iomanip>

// From regressor.cpp
void H_func(double *regressor, const double *q, const double *dq,
            const double *ddq);

int main() {
    std::cout << "=== Testing Regressor Values ===" << std::endl;

    // Test with some sample values
    double q[6] = {0.1, 0.2, 0.3, 0.4, 0.5, 0.6};
    double dq[6] = {0.01, 0.02, 0.03, 0.04, 0.05, 0.06};
    double ddq[6] = {0.001, 0.002, 0.003, 0.004, 0.005, 0.006};

    double regressor[504];
    H_func(regressor, q, dq, ddq);

    std::cout << "Sample regressor values:" << std::endl;
    std::cout << "First 10 values: ";
    for (int i = 0; i < 10; i++) {
        std::cout << std::scientific << std::setprecision(3) << regressor[i] << " ";
    }
    std::cout << std::endl;

    std::cout << "Values for joint 0 (indices 0-83):" << std::endl;
    for (int i = 0; i < 10; i++) {
        std::cout << i << ": " << std::scientific << std::setprecision(3) << regressor[i] << std::endl;
    }

    std::cout << "Values for joint 0, some specific indices:" << std::endl;
    int base_idxs[] = {5, 6, 7, 11, 12, 13, 14, 15, 16, 18};
    for (int i = 0; i < 10; i++) {
        int idx = base_idxs[i];
        std::cout << "idx " << idx << ": " << std::scientific << std::setprecision(3) << regressor[idx] << std::endl;
    }

    return 0;
}