#include <array>
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <thread>
#include <vector>

struct Employee {
    const char* id;
    const char* name;
    const char* department;
    long baseSalary;
    long allowance;
    int overtimeHours;
};

struct DepartmentTotal {
    int employees = 0;
    long netPay = 0;
    long companyCost = 0;
};

struct PayrollRecord {
    unsigned int employeeNumber;
    unsigned int baseSalary;
    unsigned int allowance;
    unsigned short overtimeHours;
    unsigned char profileIndex;
};

static bool validMonth(const std::string& month) {
    if (month.size() != 7 || month[4] != '-') return false;
    for (size_t i = 0; i < month.size(); ++i) {
        if (i == 4) continue;
        if (month[i] < '0' || month[i] > '9') return false;
    }
    int mm = std::atoi(month.substr(5, 2).c_str());
    return mm >= 1 && mm <= 12;
}

// Demo-only progressive tax calculation; these brackets are illustrative, not tax advice.
static long demoIncomeTax(long taxableIncome) {
    const long firstBand = 10000000;
    const long secondBand = 20000000;
    long tax = 0;
    if (taxableIncome > 0) tax += (taxableIncome < firstBand ? taxableIncome : firstBand) * 5 / 100;
    if (taxableIncome > firstBand) {
        const long amount = taxableIncome - firstBand < secondBand - firstBand
            ? taxableIncome - firstBand : secondBand - firstBand;
        tax += amount * 10 / 100;
    }
    if (taxableIncome > secondBand) tax += (taxableIncome - secondBand) * 20 / 100;
    return tax;
}

int main(int argc, char** argv) {
    const auto startedAt = std::chrono::steady_clock::now();
    std::string month = "2026-10";
    int bonusPercent = 0;
    int durationSeconds = 0;
    int cpuPercent = 0;
    int memoryMb = 0;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--month") == 0 && i + 1 < argc) {
            month = argv[++i];
            if (!validMonth(month)) {
                std::cerr << "--month must use YYYY-MM format\n";
                return 2;
            }
        } else if (std::strcmp(argv[i], "--bonus-percent") == 0 && i + 1 < argc) {
            char* end = nullptr;
            long parsed = std::strtol(argv[++i], &end, 10);
            if (end == argv[i] || *end != '\0' || parsed < 0 || parsed > 100) {
                std::cerr << "--bonus-percent must be between 0 and 100\n";
                return 2;
            }
            bonusPercent = static_cast<int>(parsed);
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
        } else {
            std::cerr << "Usage: payroll_job [--month YYYY-MM] [--bonus-percent 0..100] [--duration-seconds 0..120] [--cpu-percent 0..100] [--memory-mb 0..256]\n";
            return 2;
        }
    }

    if (durationSeconds == 0 && (cpuPercent > 0 || memoryMb > 0)) {
        std::cerr << "Set --duration-seconds when requesting CPU or RAM workload\n";
        return 2;
    }

    // Synthetic employee records for demonstration only.
    const std::array<Employee, 12> employees = {{
        {"E1001", "An Nguyen", "Engineering", 28000000, 2500000, 6},
        {"E1002", "Binh Tran", "Engineering", 22500000, 1800000, 4},
        {"E1003", "Chi Le", "Engineering", 19500000, 1500000, 8},
        {"E2001", "Dung Pham", "Sales", 18000000, 3500000, 10},
        {"E2002", "Giang Vo", "Sales", 16500000, 3000000, 7},
        {"E2003", "Hanh Do", "Sales", 15000000, 2800000, 5},
        {"E3001", "Khanh Bui", "Finance", 21000000, 2000000, 3},
        {"E3002", "Lan Ho", "Finance", 17500000, 1700000, 2},
        {"E4001", "Minh Phan", "Operations", 16000000, 2200000, 9},
        {"E4002", "Nam Dang", "Operations", 14500000, 1800000, 6},
        {"E5001", "Oanh Vu", "HumanResources", 17000000, 2000000, 1},
        {"E5002", "Phuc Ly", "HumanResources", 15500000, 1600000, 0}
    }};

    // Generate a growing synthetic payroll ledger; each record stays resident until report creation.
    const size_t memoryTargetRecords = static_cast<size_t>(memoryMb) * 1024 * 1024 / sizeof(PayrollRecord);
    const size_t targetRecords = durationSeconds > 0 && memoryTargetRecords > 0
        ? memoryTargetRecords : employees.size();
    std::vector<PayrollRecord> payrollRecords;
    payrollRecords.reserve(targetRecords);
    auto appendPayrollRecordsUntil = [&](size_t target) {
        while (payrollRecords.size() < target) {
            const unsigned int number = static_cast<unsigned int>(payrollRecords.size() + 1);
            const unsigned char profileIndex = static_cast<unsigned char>((number - 1) % employees.size());
            const Employee& profile = employees[profileIndex];
            payrollRecords.push_back({number,
                static_cast<unsigned int>(profile.baseSalary + (number % 5) * 100000),
                static_cast<unsigned int>(profile.allowance),
                static_cast<unsigned short>(profile.overtimeHours + number % 3), profileIndex});
        }
    };

    if (durationSeconds > 0) {
        const auto deadline = startedAt + std::chrono::seconds(durationSeconds);
        const auto cycle = std::chrono::milliseconds(100);
        while (std::chrono::steady_clock::now() < deadline) {
            const auto cycleStart = std::chrono::steady_clock::now();
            const double progress = std::chrono::duration<double>(cycleStart - startedAt).count() / durationSeconds;
            const double ramp = progress > 1.0 ? 1.0 : progress;
            appendPayrollRecordsUntil(static_cast<size_t>(targetRecords * ramp));

            const int cycleCpuPercent = static_cast<int>(cpuPercent * ramp);
            const auto busyTime = std::chrono::duration_cast<std::chrono::steady_clock::duration>(
                cycle * (cycleCpuPercent / 100.0));
            const auto busyUntil = cycleStart + busyTime;
            volatile std::uint64_t checksum = 0;
            while (std::chrono::steady_clock::now() < busyUntil && !payrollRecords.empty()) {
                std::uint64_t batchNet = 0;
                const size_t scanCount = std::min<size_t>(payrollRecords.size(), 4096);
                const size_t begin = payrollRecords.size() - scanCount;
                for (size_t i = begin; i < payrollRecords.size(); ++i) {
                    const PayrollRecord& record = payrollRecords[i];
                    const long gross = record.baseSalary + record.allowance
                        + static_cast<long>(record.baseSalary) * record.overtimeHours * 3 / 208
                        + static_cast<long>(record.baseSalary) * bonusPercent / 100;
                    const long insurance = static_cast<long>(record.baseSalary) * 105 / 1000;
                    const long net = gross - insurance - demoIncomeTax(gross - insurance);
                    batchNet += static_cast<std::uint64_t>(net + record.employeeNumber);
                }
                checksum = checksum + batchNet;
            }
            if (cycleCpuPercent < 100) std::this_thread::sleep_until(cycleStart + cycle);
        }
        appendPayrollRecordsUntil(targetRecords);
    } else {
        appendPayrollRecordsUntil(targetRecords);
    }

    long long batchGrossTotal = 0;
    long long batchNetTotal = 0;
    for (const PayrollRecord& record : payrollRecords) {
        const long gross = record.baseSalary + record.allowance
            + static_cast<long>(record.baseSalary) * record.overtimeHours * 3 / 208
            + static_cast<long>(record.baseSalary) * bonusPercent / 100;
        const long bhxh = static_cast<long>(record.baseSalary) * 8 / 100;
        const long bhyt = static_cast<long>(record.baseSalary) * 15 / 1000;
        const long bhtn = record.baseSalary / 100;
        batchGrossTotal += gross;
        batchNetTotal += gross - bhxh - bhyt - bhtn - demoIncomeTax(gross - bhxh - bhyt - bhtn);
    }

    const char* configuredDir = std::getenv("JOB_OUTPUT_DIR");
    const std::string outputDir = configuredDir ? configuredDir : ".";
    std::ofstream csv(outputDir + "/result.csv");
    std::ofstream json(outputDir + "/result.json");
    if (!csv || !json) {
        std::cerr << "Cannot create payroll output files in " << outputDir << "\n";
        return 1;
    }

    csv << "employee_id,name,department,base_salary_vnd,bonus_vnd,allowance_vnd,overtime_hours,overtime_pay_vnd,gross_vnd,bhxh_vnd,bhyt_vnd,bhtn_vnd,pit_demo_vnd,net_pay_vnd,company_cost_vnd\n";
    long totalGross = 0;
    long totalSocial = 0;
    long totalHealth = 0;
    long totalUnemployment = 0;
    long totalPit = 0;
    long totalNet = 0;
    long totalCompanyCost = 0;
    std::map<std::string, DepartmentTotal> departments;
    const size_t csvSampleCount = std::min<size_t>(payrollRecords.size(), 1000);
    const size_t jsonSampleCount = std::min<size_t>(payrollRecords.size(), 100);

    json << "{\n  \"report_type\": \"monthly_payroll_demo\",\n"
         << "  \"month\": \"" << month << "\",\n"
         << "  \"bonus_percent\": " << bonusPercent << ",\n"
         << "  \"demo_duration_seconds\": " << durationSeconds << ",\n"
         << "  \"demo_cpu_percent_one_core\": " << cpuPercent << ",\n"
         << "  \"demo_memory_mb\": " << memoryMb << ",\n"
         << "  \"tax_note\": \"Illustrative demo brackets only; not official payroll tax calculation\",\n"
         << "  \"employees\": [\n";

    for (size_t i = 0; i < payrollRecords.size(); ++i) {
        const PayrollRecord& record = payrollRecords[i];
        const Employee& employee = employees[record.profileIndex];
        const long bonus = static_cast<long>(record.baseSalary) * bonusPercent / 100;
        const long overtimePay = static_cast<long>(record.baseSalary) * record.overtimeHours * 3 / 208;
        const long gross = record.baseSalary + bonus + record.allowance + overtimePay;
        const long bhxh = static_cast<long>(record.baseSalary) * 8 / 100;
        const long bhyt = static_cast<long>(record.baseSalary) * 15 / 1000;
        const long bhtn = record.baseSalary / 100;
        const long pit = demoIncomeTax(gross - bhxh - bhyt - bhtn);
        const long net = gross - bhxh - bhyt - bhtn - pit;
        const long companyCost = gross + static_cast<long>(record.baseSalary) * 215 / 1000;
        char employeeId[24];
        std::snprintf(employeeId, sizeof(employeeId), "E%06u", record.employeeNumber);

        if (i < csvSampleCount) {
            csv << employeeId << ',' << employee.name << ',' << employee.department << ','
                << record.baseSalary << ',' << bonus << ',' << record.allowance << ','
                << record.overtimeHours << ',' << overtimePay << ',' << gross << ','
                << bhxh << ',' << bhyt << ',' << bhtn << ',' << pit << ',' << net << ','
                << companyCost << '\n';
        }

        if (i < jsonSampleCount) {
            json << "    {\"employee_id\": \"" << employeeId << "\", \"name\": \""
                 << employee.name << "\", \"department\": \"" << employee.department
                 << "\", \"gross_vnd\": " << gross << ", \"deductions_vnd\": "
                 << (bhxh + bhyt + bhtn + pit) << ", \"net_pay_vnd\": " << net
                 << ", \"company_cost_vnd\": " << companyCost << "}"
                 << (i + 1 == jsonSampleCount ? "\n" : ",\n");
        }

        DepartmentTotal& department = departments[employee.department];
        department.employees++;
        department.netPay += net;
        department.companyCost += companyCost;
        totalGross += gross;
        totalSocial += bhxh;
        totalHealth += bhyt;
        totalUnemployment += bhtn;
        totalPit += pit;
        totalNet += net;
        totalCompanyCost += companyCost;
    }

    json << "  ],\n  \"employee_detail_rows\": " << jsonSampleCount
         << ",\n  \"csv_detail_rows\": " << csvSampleCount
         << ",\n  \"batch_ledger\": {\"employees_processed\": " << payrollRecords.size()
         << ", \"gross_payroll_vnd\": " << batchGrossTotal
         << ", \"net_payroll_vnd\": " << batchNetTotal << "},\n"
         << "  \"totals\": {\"employee_count\": " << payrollRecords.size()
         << ", \"gross_vnd\": " << totalGross << ", \"bhxh_vnd\": " << totalSocial
         << ", \"bhyt_vnd\": " << totalHealth << ", \"bhtn_vnd\": " << totalUnemployment
         << ", \"pit_demo_vnd\": " << totalPit << ", \"net_pay_vnd\": " << totalNet
         << ", \"company_cost_vnd\": " << totalCompanyCost << "},\n"
         << "  \"by_department\": [\n";

    size_t departmentIndex = 0;
    for (const auto& item : departments) {
        json << "    {\"department\": \"" << item.first << "\", \"employees\": "
             << item.second.employees << ", \"net_pay_vnd\": " << item.second.netPay
             << ", \"company_cost_vnd\": " << item.second.companyCost << "}"
             << (++departmentIndex == departments.size() ? "\n" : ",\n");
    }
    json << "  ]\n}\n";
    if (!csv || !json) {
        std::cerr << "Failed while writing payroll report\n";
        return 1;
    }
    std::cout << "Payroll report ready: " << payrollRecords.size() << " batch records; sample payroll net "
              << totalNet << " VND; files: result.csv, result.json\n";
    return 0;
}
