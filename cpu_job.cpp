
#include <chrono>
#include <cstdlib>
#include <iostream>

// Su dung CPU trong N giay
// Cach dung: cpu_job <so_giay>

int main(int argc, char* argv[]) {
    int giay = 30;

    if (argc > 1) {
        char* end = nullptr;
        long n = std::strtol(argv[1], &end, 10);

        if (end == argv[1] || *end != '\0' || n < 1 || n > 600) {
            std::cerr << "Usage: cpu_job <seconds: 1-600>\n";
            return 1;
        }

        giay = static_cast<int>(n);
    }

    std::cout << "cpu_job started for "
              << giay << " seconds\n";

    const auto start = std::chrono::steady_clock::now();
    volatile unsigned long long x = 0;

    while (std::chrono::steady_clock::now() - start
           < std::chrono::seconds(giay)) {
        x = x + 1;
    }

    std::cout << "cpu_job finished\n";
    return 0;
}
