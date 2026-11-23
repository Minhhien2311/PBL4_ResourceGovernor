#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <string>
#include <vector>
#include <unordered_map>
#include <map>
#include <queue>
#include <random>

struct GiaoDich {
    std::string id;
    std::string taiKhoan;
    std::string chiNhanh;
    std::string loaiTien;
    long soTien;
    time_t thoiGian;
    std::string moTa;
};

// Cấu trúc để lưu lỗi (dùng cho Min-Heap lấy Top K)
struct LoiDoiSoat {
    std::string id;
    std::string chiNhanh;
    std::string loaiLoi;
    long soTienNH;
    long soTienSS;
    long chenhLech;
    long absChenhLech;
};

// Cấu trúc thống kê tổng hợp
struct ThongKeChiNhanh {
    long soLoi = 0;
    long tongTienLech = 0;
};

std::string generateMoTa(std::mt19937& rng) {
    static const char* words[] = {"Thanh toan", "Chuyen khoan", "Rut tien", "Gui tien", "Phi dich vu", "Hoan tien", "Mua hang", "Tra luong"};
    std::uniform_int_distribution<int> dist(0, 7);
    return words[dist(rng)];
}

int main(int argc, char** argv) {
    int n = 3000000; // 3 triệu giao dịch (ngốn RAM)
    unsigned int seed = 42;
    int topK = 1000; // Mặc định lấy Top 1000 lỗi lớn nhất

    for (int i = 1; i < argc; i++) {
        if (std::strcmp(argv[i], "--n") == 0 && i + 1 < argc) n = std::atoi(argv[++i]);
        else if (std::strcmp(argv[i], "--seed") == 0 && i + 1 < argc) seed = std::atoi(argv[++i]);
        else if (std::strcmp(argv[i], "--top-k") == 0 && i + 1 < argc) topK = std::atoi(argv[++i]);
    }
    if (n < 1000) n = 1000;
    if (n > 10000000) n = 10000000;
    if (topK < 10) topK = 10;
    if (topK > 5000) topK = 5000;

    const char* outDir = std::getenv("JOB_OUTPUT_DIR");
    std::string basePath = outDir ? outDir : ".";
    std::string csvPath = basePath + "/result.csv";
    std::string jsonPath = basePath + "/summary.json";

    printf("doi_soat_job: n=%d seed=%u topK=%d -> %s\n", n, seed, topK, basePath.c_str());
    fflush(stdout);

    std::mt19937 rng(seed);
    std::uniform_int_distribution<long> distTien(1000, 50000000);
    std::uniform_int_distribution<int> distChiNhanh(1, 100);
    std::uniform_int_distribution<int> distLoai(0, 3);
    std::uniform_int_distribution<int> distTime(0, 30 * 24 * 3600);
    time_t batDau = time(nullptr) - 30 * 24 * 3600;

    // 1. Sinh dữ liệu ngân hàng (RAM Heavy)
    std::vector<GiaoDich> nganHang;
    nganHang.reserve(n);
    char buf[64];

    for (int i = 0; i < n; i++) {
        GiaoDich g;
        std::snprintf(buf, sizeof(buf), "TX%09d", i + 1);
        g.id = buf;
        std::snprintf(buf, sizeof(buf), "ACC%07d", (i % 1000000) + 1);
        g.taiKhoan = buf;
        std::snprintf(buf, sizeof(buf), "CN%03d", distChiNhanh(rng));
        g.chiNhanh = buf;
        g.loaiTien = (distLoai(rng) == 0) ? "USD" : "VND";
        g.soTien = distTien(rng);
        g.thoiGian = batDau + distTime(rng);
        g.moTa = generateMoTa(rng);
        nganHang.push_back(std::move(g));
    }

    // 2. Sinh dữ liệu sổ sách (có sai lệch)
    std::vector<GiaoDich> soSach;
    soSach.reserve(n + n / 100);
    
    for (int i = 0; i < n; i++) {
        int loai = distChiNhanh(rng) % 100; 
        if (loai < 2) continue; // 2% thiếu trong sổ sách
        GiaoDich g = nganHang[i];
        if (loai < 8) g.soTien += (distTien(rng) % 10000000); // 6% lệch tiền
        soSach.push_back(std::move(g));
    }
    // 1% giao dịch thừa (có trong sổ sách nhưng không có trong ngân hàng)
    int thua = n / 100;
    for (int i = 0; i < thua; i++) {
        GiaoDich g;
        std::snprintf(buf, sizeof(buf), "TX9%08d", i);
        g.id = buf;
        std::snprintf(buf, sizeof(buf), "ACC%07d", distChiNhanh(rng));
        g.taiKhoan = buf;
        std::snprintf(buf, sizeof(buf), "CN%03d", distChiNhanh(rng));
        g.chiNhanh = buf;
        g.loaiTien = "VND";
        g.soTien = distTien(rng);
        g.thoiGian = batDau + distTime(rng);
        g.moTa = generateMoTa(rng);
        soSach.push_back(std::move(g));
    }

    // 3. Đốt CPU (Hash checksum) để job chạy lâu hơn (phục vụ demo)
    volatile size_t hashResult = 0;
    for (const auto& g : nganHang) {
        unsigned int sum = 0;
        for (char c : g.id) sum = (sum * 31) ^ c;
        for (char c : g.taiKhoan) sum = (sum * 31) ^ c;
        hashResult += sum;
    }

    // 4. Sort sổ sách để đối soát (Two-pointer)
    std::sort(soSach.begin(), soSach.end(), [](const GiaoDich& a, const GiaoDich& b) { return a.id < b.id; });

    // 5. Cấu trúc dữ liệu cho Top-K và Summary
    auto cmp = [](const LoiDoiSoat& a, const LoiDoiSoat& b) {
        return a.absChenhLech > b.absChenhLech; // Min-heap: phần tử nhỏ nhất nằm trên cùng
    };
    std::priority_queue<LoiDoiSoat, std::vector<LoiDoiSoat>, decltype(cmp)> topKLoi(cmp);
    std::map<std::string, ThongKeChiNhanh> summary;

    long tongMismatch = 0;
    size_t i1 = 0, i2 = 0;

    // Hàm helper để xử lý việc push vào Top-K và Summary
    auto addError = [&](const std::string& id, const std::string& cn, const std::string& loai, long nh, long ss, long chenh) {
        tongMismatch++;
        
        // Cập nhật Summary
        summary[cn].soLoi++;
        summary[cn].tongTienLech += std::abs(chenh);

        // Cập nhật Top-K
        LoiDoiSoat loi{id, cn, loai, nh, ss, chenh, std::abs(chenh)};
        if ((int)topKLoi.size() < topK) {
            topKLoi.push(loi);
        } else if (loi.absChenhLech > topKLoi.top().absChenhLech) {
            topKLoi.pop();
            topKLoi.push(loi);
        }
    };

    // Thuật toán đối soát 2 con trỏ
    while (i1 < nganHang.size() && i2 < soSach.size()) {
        if (nganHang[i1].id == soSach[i2].id) {
            long chenh = soSach[i2].soTien - nganHang[i1].soTien;
            if (chenh != 0) {
                addError(soSach[i2].id, soSach[i2].chiNhanh, "LECH_SO_TIEN", nganHang[i1].soTien, soSach[i2].soTien, chenh);
            }
            i1++; i2++;
        } else if (nganHang[i1].id < soSach[i2].id) {
            addError(nganHang[i1].id, nganHang[i1].chiNhanh, "THIEU_TRONG_SO_SACH", nganHang[i1].soTien, 0, nganHang[i1].soTien);
            i1++;
        } else {
            addError(soSach[i2].id, soSach[i2].chiNhanh, "THIEU_TRONG_NGAN_HANG", 0, soSach[i2].soTien, -soSach[i2].soTien);
            i2++;
        }
    }
    while (i1 < nganHang.size()) {
        addError(nganHang[i1].id, nganHang[i1].chiNhanh, "THIEU_TRONG_SO_SACH", nganHang[i1].soTien, 0, nganHang[i1].soTien);
        i1++;
    }
    while (i2 < soSach.size()) {
        addError(soSach[i2].id, soSach[i2].chiNhanh, "THIEU_TRONG_NGAN_HANG", 0, soSach[i2].soTien, -soSach[i2].soTien);
        i2++;
    }

    // 6. Ghi file Result.csv (Chỉ ghi Top-K lỗi lớn nhất)
    FILE* fCsv = std::fopen(csvPath.c_str(), "w");
    if (fCsv) {
        std::fprintf(fCsv, "ma_giao_dich,chi_nhanh,loai_loi,so_tien_ngan_hang,so_tien_so_sach,chenh_lech\n");
        
        // Đưa priority_queue ra vector và sort giảm dần
        std::vector<LoiDoiSoat> listLoi;
        while (!topKLoi.empty()) {
            listLoi.push_back(topKLoi.top());
            topKLoi.pop();
        }
        std::sort(listLoi.begin(), listLoi.end(), [](const LoiDoiSoat& a, const LoiDoiSoat& b) {
            return a.absChenhLech > b.absChenhLech;
        });

        for (const auto& loi : listLoi) {
            std::fprintf(fCsv, "%s,%s,%s,%ld,%ld,%ld\n", loi.id.c_str(), loi.chiNhanh.c_str(), loi.loaiLoi.c_str(), loi.soTienNH, loi.soTienSS, loi.chenhLech);
        }
        std::fclose(fCsv);
    }

    // 7. Ghi file Summary.json (Thống kê theo chi nhánh)
    FILE* fJson = std::fopen(jsonPath.c_str(), "w");
    if (fJson) {
        std::fprintf(fJson, "{\n");
        std::fprintf(fJson, "  \"tong_giao_dich\": %d,\n", n);
        std::fprintf(fJson, "  \"tong_so_loi\": %ld,\n", tongMismatch);
        std::fprintf(fJson, "  \"chi_tiet_chi_nhanh\": [\n");
        
        int count = 0;
        for (const auto& kv : summary) {
            std::fprintf(fJson, "    {\"chi_nhanh\": \"%s\", \"so_loi\": %ld, \"tong_lech\": %ld}%s\n", 
                kv.first.c_str(), kv.second.soLoi, kv.second.tongTienLech, 
                (++count == summary.size() ? "" : ","));
        }
        std::fprintf(fJson, "  ]\n}\n");
        std::fclose(fJson);
    }

    printf("doi_soat_job: xong. Tong loi=%ld. Ghi %d loi vao CSV. hash=%zu\n", tongMismatch, topK, (size_t)hashResult);
    return 0;
}