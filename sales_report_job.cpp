#include <array>
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <random>
#include <string>
#include <thread>
#include <vector>

struct Product {
    const char* name;
    long unitPrice;
};

struct ProductTotal {
    long orders = 0;
    long units = 0;
    long revenue = 0;
};

struct SalesTransaction {
    unsigned int gross;
    unsigned int discount;
    unsigned int net;
    unsigned short productIndex;
    unsigned char regionIndex;
    unsigned char quantity;
    unsigned char day;
};

static bool readPositiveInt(const char* text, int& value) {
    char* end = nullptr;
    long parsed = std::strtol(text, &end, 10);
    if (end == text || *end != '\0' || parsed < 1 || parsed > 100000) return false;
    value = static_cast<int>(parsed);
    return true;
}

static bool validMonth(const std::string& month) {
    if (month.size() != 7 || month[4] != '-') return false;
    for (size_t i = 0; i < month.size(); ++i) {
        if (i == 4) continue;
        if (month[i] < '0' || month[i] > '9') return false;
    }
    int mm = std::atoi(month.substr(5, 2).c_str());
    return mm >= 1 && mm <= 12;
}

int main(int argc, char** argv) {
    const auto startedAt = std::chrono::steady_clock::now();
    int orders = 24;
    int durationSeconds = 0;
    int cpuPercent = 0;
    int memoryMb = 0;
    unsigned int seed = 42;
    std::string month = "2026-10";

    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--orders") == 0 && i + 1 < argc) {
            if (!readPositiveInt(argv[++i], orders)) {
                std::cerr << "--orders must be between 1 and 100000\n";
                return 2;
            }
        } else if (std::strcmp(argv[i], "--seed") == 0 && i + 1 < argc) {
            int parsed = 0;
            if (!readPositiveInt(argv[++i], parsed)) {
                std::cerr << "--seed must be between 1 and 100000\n";
                return 2;
            }
            seed = static_cast<unsigned int>(parsed);
        } else if (std::strcmp(argv[i], "--duration-seconds") == 0 && i + 1 < argc) {
            char* end = nullptr;
            long parsed = std::strtol(argv[++i], &end, 10);
            if (end == argv[i] || *end != '\0' || parsed < 0 || parsed > 120) {
                std::cerr << "--duration-seconds must be between 0 and 120\n";
                return 2;
            }
            durationSeconds = static_cast<int>(parsed);
        } else if (std::strcmp(argv[i], "--cpu-percent") == 0 && i + 1 < argc) {
            char* end = nullptr;
            long parsed = std::strtol(argv[++i], &end, 10);
            if (end == argv[i] || *end != '\0' || parsed < 0 || parsed > 100) {
                std::cerr << "--cpu-percent must be between 0 and 100\n";
                return 2;
            }
            cpuPercent = static_cast<int>(parsed);
        } else if (std::strcmp(argv[i], "--memory-mb") == 0 && i + 1 < argc) {
            char* end = nullptr;
            long parsed = std::strtol(argv[++i], &end, 10);
            if (end == argv[i] || *end != '\0' || parsed < 0 || parsed > 256) {
                std::cerr << "--memory-mb must be between 0 and 256\n";
                return 2;
            }
            memoryMb = static_cast<int>(parsed);
        } else if (std::strcmp(argv[i], "--month") == 0 && i + 1 < argc) {
            month = argv[++i];
            if (!validMonth(month)) {
                std::cerr << "--month must use YYYY-MM format\n";
                return 2;
            }
        } else {
            std::cerr << "Usage: sales_report_job [--month YYYY-MM] [--orders 1..100000] [--seed N] [--duration-seconds 0..120] [--cpu-percent 0..100] [--memory-mb 0..256]\n";
            return 2;
        }
    }

    if (durationSeconds == 0 && (cpuPercent > 0 || memoryMb > 0)) {
        std::cerr << "Set --duration-seconds when requesting CPU or RAM workload\n";
        return 2;
    }

    const std::array<Product, 5> products = {{{"Laptop", 18500000}, {"Phone", 12500000},
        {"Monitor", 4200000}, {"Keyboard", 950000}, {"Service", 2500000}}};
    const std::array<const char*, 4> regions = {{"North", "Central", "South", "Online"}};
    std::array<ProductTotal, 5> productTotals{};
    std::array<long, 4> regionTotals{};
    std::array<long, 4> regionOrders{};
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> productPick(0, static_cast<int>(products.size()) - 1);
    std::uniform_int_distribution<int> regionPick(0, static_cast<int>(regions.size()) - 1);
    std::uniform_int_distribution<int> quantityPick(1, 4);
    std::uniform_int_distribution<int> dayPick(1, 28);

    const size_t memoryTargetRecords = static_cast<size_t>(memoryMb) * 1024 * 1024 / sizeof(SalesTransaction);
    const size_t targetRecords = durationSeconds > 0 && memoryTargetRecords > 0
        ? memoryTargetRecords : static_cast<size_t>(orders);
    std::vector<SalesTransaction> transactions;
    transactions.reserve(targetRecords);

    auto appendTransactionsUntil = [&](size_t target) {
        while (transactions.size() < target) {
            const int productIndex = productPick(rng);
            const int regionIndex = regionPick(rng);
            const int quantity = quantityPick(rng);
            const long gross = products[productIndex].unitPrice * quantity;
            const long discount = gross >= 20000000 ? gross / 20 : 0;
            transactions.push_back({static_cast<unsigned int>(gross), static_cast<unsigned int>(discount),
                static_cast<unsigned int>(gross - discount), static_cast<unsigned short>(productIndex),
                static_cast<unsigned char>(regionIndex), static_cast<unsigned char>(quantity),
                static_cast<unsigned char>(dayPick(rng))});
        }
    };

    if (durationSeconds > 0) {
        const auto deadline = startedAt + std::chrono::seconds(durationSeconds);
        const auto cycle = std::chrono::milliseconds(100);
        while (std::chrono::steady_clock::now() < deadline) {
            const auto cycleStart = std::chrono::steady_clock::now();
            const double progress = std::chrono::duration<double>(cycleStart - startedAt).count() / durationSeconds;
            const size_t targetNow = static_cast<size_t>(targetRecords * (progress > 1.0 ? 1.0 : progress));
            appendTransactionsUntil(targetNow);

            const int cycleCpuPercent = static_cast<int>(cpuPercent * (progress > 1.0 ? 1.0 : progress));
            const auto busyTime = std::chrono::duration_cast<std::chrono::steady_clock::duration>(
                cycle * (cycleCpuPercent / 100.0));
            const auto busyUntil = cycleStart + busyTime;
            volatile std::uint64_t checksum = 0;
            while (std::chrono::steady_clock::now() < busyUntil && !transactions.empty()) {
                std::uint64_t batchNet = 0;
                const size_t scanCount = std::min<size_t>(transactions.size(), 65536);
                const size_t begin = transactions.size() - scanCount;
                for (size_t i = begin; i < transactions.size(); ++i) {
                    const SalesTransaction& t = transactions[i];
                    batchNet += t.gross - t.discount + (t.productIndex + t.regionIndex + t.quantity + t.day);
                }
                checksum = checksum + batchNet;
            }
            if (cycleCpuPercent < 100) std::this_thread::sleep_until(cycleStart + cycle);
        }
        appendTransactionsUntil(targetRecords);
    } else {
        appendTransactionsUntil(targetRecords);
    }

    const char* configuredDir = std::getenv("JOB_OUTPUT_DIR");
    const std::string outputDir = configuredDir ? configuredDir : ".";
    std::ofstream csv(outputDir + "/result.csv");
    std::ofstream json(outputDir + "/result.json");
    if (!csv || !json) {
        std::cerr << "Cannot create sales report output files in " << outputDir << "\n";
        return 1;
    }

    csv << "order_id,date,region,product,quantity,unit_price_vnd,gross_vnd,discount_vnd,net_vnd\n";
    long totalGross = 0;
    long totalDiscount = 0;
    long totalNet = 0;

    const size_t csvSampleCount = std::min<size_t>(transactions.size(), 1000);
    for (size_t i = 0; i < transactions.size(); ++i) {
        const SalesTransaction& transaction = transactions[i];
        const size_t productIndex = transaction.productIndex;
        const size_t regionIndex = transaction.regionIndex;
        const int quantity = transaction.quantity;
        const long gross = transaction.gross;
        const long discount = transaction.discount;
        const long net = transaction.net;
        char orderId[32];
        char date[16];
        std::snprintf(orderId, sizeof(orderId), "SO-%s-%06u", month.c_str(), static_cast<unsigned int>(i + 1));
        std::snprintf(date, sizeof(date), "%s-%02u", month.c_str(), static_cast<unsigned int>(transaction.day));

        if (i < csvSampleCount) {
            csv << orderId << ',' << date << ',' << regions[regionIndex] << ','
                << products[productIndex].name << ',' << quantity << ','
                << products[productIndex].unitPrice << ',' << gross << ',' << discount << ',' << net << '\n';
        }

        ProductTotal& productTotal = productTotals[productIndex];
        productTotal.orders++;
        productTotal.units += quantity;
        productTotal.revenue += net;
        regionOrders[regionIndex]++;
        regionTotals[regionIndex] += net;
        totalGross += gross;
        totalDiscount += discount;
        totalNet += net;
    }

    size_t bestProduct = 0;
    for (size_t i = 1; i < productTotals.size(); ++i) {
        if (productTotals[i].revenue > productTotals[bestProduct].revenue) bestProduct = i;
    }

    json << "{\n  \"report_type\": \"monthly_sales_demo\",\n"
         << "  \"month\": \"" << month << "\",\n"
         << "  \"orders\": " << transactions.size() << ",\n"
         << "  \"csv_detail_rows\": " << csvSampleCount << ",\n"
         << "  \"demo_duration_seconds\": " << durationSeconds << ",\n"
         << "  \"demo_cpu_percent_one_core\": " << cpuPercent << ",\n"
         << "  \"demo_memory_mb\": " << memoryMb << ",\n"
         << "  \"gross_revenue_vnd\": " << totalGross << ",\n"
         << "  \"discount_vnd\": " << totalDiscount << ",\n"
         << "  \"net_revenue_vnd\": " << totalNet << ",\n"
         << "  \"top_product\": \"" << products[bestProduct].name << "\",\n"
         << "  \"by_product\": [\n";
    for (size_t i = 0; i < products.size(); ++i) {
        json << "    {\"product\": \"" << products[i].name << "\", \"orders\": "
             << productTotals[i].orders << ", \"units\": " << productTotals[i].units
             << ", \"net_revenue_vnd\": " << productTotals[i].revenue << "}"
             << (i + 1 == products.size() ? "\n" : ",\n");
    }
    json << "  ],\n  \"by_region\": [\n";
    for (size_t i = 0; i < regions.size(); ++i) {
        json << "    {\"region\": \"" << regions[i] << "\", \"orders\": "
             << regionOrders[i] << ", \"net_revenue_vnd\": " << regionTotals[i] << "}"
             << (i + 1 == regions.size() ? "\n" : ",\n");
    }
    json << "  ]\n}\n";

    if (!csv || !json) {
        std::cerr << "Failed while writing sales report\n";
        return 1;
    }
    std::cout << "Sales report ready: " << transactions.size() << " orders, net revenue "
              << totalNet << " VND; files: result.csv, result.json\n";
    return 0;
}
