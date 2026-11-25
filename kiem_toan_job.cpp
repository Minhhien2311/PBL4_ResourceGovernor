#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <string>
#include <vector>
#include <deque>
#include <random>
#include <algorithm>
#include <chrono>

int main(int argc, char** argv) {
    int n = 10000000; // 10 triệu giao dịch mặc định
    int durationSeconds = 45; // Thời gian tối thiểu để dễ quan sát mức CPU
    unsigned int seed = 42;

    for (int i = 1; i < argc; i++) {
        if (std::strcmp(argv[i], "--n") == 0 && i + 1 < argc) n = std::atoi(argv[++i]);
        else if (std::strcmp(argv[i], "--seed") == 0 && i + 1 < argc) seed = std::atoi(argv[++i]);
        else if (std::strcmp(argv[i], "--duration-seconds") == 0 && i + 1 < argc)
            durationSeconds = std::atoi(argv[++i]);
    }
    if (n < 1000) n = 1000;
    if (n > 50000000) n = 50000000;
    if (durationSeconds < 30) durationSeconds = 30;
    if (durationSeconds > 60) durationSeconds = 60;

    const char* outDir = std::getenv("JOB_OUTPUT_DIR");
    std::string outPath = outDir ? (std::string(outDir) + "/result.json") : "result.json";

    printf("kiem_toan_job: n=%d seed=%u -> %s\n", n, seed, outPath.c_str());
    fflush(stdout);

    const auto t0 = std::chrono::steady_clock::now();
    std::mt19937 rng(seed);
    std::uniform_int_distribution<long> distTien(1000, 500000000);
    std::uniform_int_distribution<int> distGio(0, 23);

    long benfordCounts[10] = {0};
    long tongNghiVan = 0;
    std::vector<std::string> topNghiVan;

    double mean = 0.0, M2 = 0.0;
    long count = 0;

    // Pass 1: Welford's algorithm để tính mean/stddev (Không tốn RAM)
    for (int i = 0; i < n; i++) {
        long soTien = distTien(rng);
        count++;
        double delta = soTien - mean;
        mean += delta / count;
        double delta2 = soTien - mean;
        M2 += delta * delta2;
    }
    double variance = M2 / count;
    double stddev = std::sqrt(variance);

    // Pass 2: Kiểm toán với Sliding Window & Complex Rules
    std::deque<long> window;
    long windowSum = 0;
    char idBuf[32];
    char msgBuf[512];

    for (int i = 0; i < n; i++) {
        long soTien = distTien(rng);
        int gio = distGio(rng);
        
        // Sliding window moving average
        window.push_back(soTien);
        windowSum += soTien;
        if (window.size() > 1000) {
            windowSum -= window.front();
            window.pop_front();
        }
        double movingAvg = (double)windowSum / window.size();
        
        // Benford Law First-Digit
        long temp = soTien;
        while (temp >= 10) temp /= 10;
        if (temp >= 1 && temp <= 9) benfordCounts[temp]++;

        // Z-Score
        double zScore = (soTien - mean) / (stddev > 0 ? stddev : 1.0);
        
        bool isZScore = (zScore > 4.0);
        bool isRound = (soTien % 10000000 == 0 && soTien >= 10000000);
        bool isOddHour = (gio >= 2 && gio < 5);
        bool isSpike = (soTien > movingAvg * 10.0 && movingAvg > 0);
        
        // Luhn Algorithm check (đốt CPU)
        std::string cardNo = std::to_string(soTien);
        if (cardNo.length() >= 8) {
            int nDigits = cardNo.length();
            int nSum = 0;
            bool isSecond = false;
            for (int k = nDigits - 1; k >= 0; k--) {
                int d = cardNo[k] - '0';
                if (isSecond) d = d * 2;
                nSum += d / 10;
                nSum += d % 10;
                isSecond = !isSecond;
            }
        }

        if (isZScore || isRound || isOddHour || isSpike) {
            tongNghiVan++;
            if (topNghiVan.size() < 500) { // Chỉ lưu top 500 để file JSON nhẹ
                std::snprintf(idBuf, sizeof(idBuf), "TX%08d", i+1);
                int offset = std::snprintf(msgBuf, sizeof(msgBuf), "{\"id\":\"%s\",\"so_tien\":%ld,\"ly_do\":[", idBuf, soTien);
                std::string s(msgBuf, offset);
                bool first = true;
                if (isZScore) { s += "\"Z_SCORE_CAO\""; first = false; }
                if (isRound) { if (!first) s += ","; s += "\"SO_TIEN_TRON\""; first = false; }
                if (isOddHour) { if (!first) s += ","; s += "\"GIO_BAT_THUONG\""; first = false; }
                if (isSpike) { if (!first) s += ","; s += "\"DOT_BIEN_DONG\""; first = false; }
                s += "]}";
                topNghiVan.push_back(s);
            }
        }
    }

    // Nếu hai lượt phân tích hoàn tất sớm, tiếp tục tính checksum CPU đến hết
    // thời lượng yêu cầu để có thể quan sát tiến trình trong lúc kiểm thử.
    volatile unsigned long long checksumLap = 0;
    unsigned long long giaTri = seed;
    while (std::chrono::steady_clock::now() - t0
           < std::chrono::seconds(durationSeconds)) {
        giaTri = giaTri * 2862933555777941757ULL + 3037000493ULL;
        checksumLap = checksumLap + ((giaTri >> 17) ^ (giaTri & 0xFFFFULL));
    }

    const double thoiGianChay =
        std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();

    FILE* f = std::fopen(outPath.c_str(), "w");
    if (!f) { perror("fopen"); return 1; }

    // Ghi file JSON
    std::fprintf(f, "{\n");
    std::fprintf(f, "  \"tong_giao_dich\": %d,\n", n);
    std::fprintf(f, "  \"so_nghi_van\": %ld,\n", tongNghiVan);
    std::fprintf(f, "  \"thoi_gian_chay\": %.1f,\n", thoiGianChay);
    std::fprintf(f, "  \"mean\": %.2f,\n", mean);
    std::fprintf(f, "  \"stddev\": %.2f,\n", stddev);
    std::fprintf(f, "  \"benford\": [");
    for (int i = 1; i <= 9; i++) {
        std::fprintf(f, "%ld%s", benfordCounts[i], i == 9 ? "" : ", ");
    }
    std::fprintf(f, "],\n");
    std::fprintf(f, "  \"top_nghi_van\": [\n    ");
    for (size_t i = 0; i < topNghiVan.size(); i++) {
        std::fprintf(f, "%s%s", topNghiVan[i].c_str(), i == topNghiVan.size() - 1 ? "" : ",\n    ");
    }
    std::fprintf(f, "\n  ]\n}\n");
    std::fclose(f);

    printf("kiem_toan_job: xong. %ld nghi van, %.1fs\n", tongNghiVan, thoiGianChay);
    return 0;
}
