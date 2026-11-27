#include "metrics.h"
#include "config.h"
#include <cctype>
#include <dirent.h>
#include <fstream>
#include <sstream>
#include <string>
#include <unistd.h>
#include <algorithm>
#include <cmath>
#include <pwd.h>
#include <vector>
#include <mutex>
#include <set>

static long s_tongCu = 0, s_rangCu = 0;
static double s_cpuCache = 0.0;
static std::mutex s_mCpu;

static std::set<std::string> docTienTrinhCanAn() {
    std::set<std::string> ketQua;
    std::istringstream danhSach(docCauHinh("ExcludeProcessNames", ""));
    std::string ten;
    while (std::getline(danhSach, ten, ',')) {
        const size_t dau = ten.find_first_not_of(" \t\r\n");
        if (dau == std::string::npos) continue;
        const size_t cuoi = ten.find_last_not_of(" \t\r\n");
        ketQua.insert(ten.substr(dau, cuoi - dau + 1));
    }
    return ketQua;
}

static bool docCpu(long& tong, long& rang) {
    std::ifstream file("/proc/stat");
    std::string dong;
    if (!std::getline(file, dong)) return false;
    std::istringstream iss(dong);
    std::string nhan;
    long user = 0, nice = 0, system = 0, idle = 0, iowait = 0, irq = 0, softirq = 0, steal = 0;
    iss >> nhan >> user >> nice >> system >> idle >> iowait >> irq >> softirq >> steal;
    tong = user + nice + system + idle + iowait + irq + softirq + steal;
    rang = idle + iowait;
    return true;
}

SoLieu laySoLieu() {
    SoLieu s{0, 0, 0, 0};

    // CPU: Lấy mẫu không block (so sánh với lần đọc trước đó)
    {
        std::lock_guard<std::mutex> lg(s_mCpu);
        long tongMoi = 0, rangMoi = 0;
        if (docCpu(tongMoi, rangMoi)) {
            if (s_tongCu == 0) {
                // Lần đầu khởi động: ngủ nhẹ 50ms (0.05s) để có mẫu ban đầu
                usleep(50000);
                long t2 = 0, r2 = 0;
                if (docCpu(t2, r2) && t2 > tongMoi) {
                    s_cpuCache = 100.0 * (double)((t2 - tongMoi) - (r2 - rangMoi)) / (double)(t2 - tongMoi);
                    tongMoi = t2;
                    rangMoi = r2;
                }
            } else if (tongMoi > s_tongCu) {
                long dTong = tongMoi - s_tongCu;
                long dRang = rangMoi - s_rangCu;
                if (dTong > 0) {
                    double pct = 100.0 * (double)(dTong - dRang) / (double)dTong;
                    if (pct < 0.0) pct = 0.0;
                    if (pct > 100.0) pct = 100.0;
                    s_cpuCache = pct;
                }
            }
            s_tongCu = tongMoi;
            s_rangCu = rangMoi;
        }
        s.cpuPhanTram = s_cpuCache;
    }

    // RAM
    std::ifstream mem("/proc/meminfo");
    std::string dong;
    long memTotal = 0, memAvailable = 0;
    while (std::getline(mem, dong)) {
        std::istringstream iss(dong);
        std::string nhan, donvi;
        if (dong.substr(0, 9) == "MemTotal:") iss >> nhan >> memTotal >> donvi;
        if (dong.substr(0, 13) == "MemAvailable:") iss >> nhan >> memAvailable >> donvi;
    }
    s.tongRamMb = memTotal / 1024;
    s.daDungRamMb = (memTotal - memAvailable) / 1024;

    // So tien trinh: dem thu muc co ten toan so trong /proc
    DIR* dir = opendir("/proc");
    if (dir != nullptr) {
        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr) {
            std::string ten = entry->d_name;
            bool laPid = !ten.empty();
            for (char c : ten) if (!isdigit(static_cast<unsigned char>(c))) { laPid = false; break; }
            if (laPid) s.soTienTrinh++;
        }
        closedir(dir);
    }
    return s;
}
static std::string tenUser(long uid) {
    struct passwd pw, *kq = nullptr;
    char buf[1024];
    if (getpwuid_r((uid_t)uid, &pw, buf, sizeof(buf), &kq) == 0 && kq != nullptr) return pw.pw_name;
    return std::to_string(uid);
}

std::vector<TienTrinhInfo> layTienTrinh(size_t toiDa) {
    std::vector<TienTrinhInfo> ds;
    const std::set<std::string> tienTrinhCanAn = docTienTrinhCanAn();
    DIR* dir = opendir("/proc");
    if (dir == nullptr) return ds;

    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        std::string ten = entry->d_name;
        bool laPid = !ten.empty();
        for (char c : ten) {
            if (!isdigit(static_cast<unsigned char>(c))) { laPid = false; break; }
        }
        if (!laPid) continue;

        std::ifstream f("/proc/" + ten + "/status");
        if (!f.is_open()) continue;

        std::string dong, name;
        long uid = -1, vmrss = -1;
        while (std::getline(f, dong)) {
            std::istringstream iss(dong);
            std::string nhan;
            if (dong.compare(0, 5, "Name:") == 0) iss >> nhan >> name;   // lay tu dau tien, giong monitor.cpp
            else if (dong.compare(0, 4, "Uid:") == 0) iss >> nhan >> uid;
            else if (dong.compare(0, 6, "VmRSS:") == 0) iss >> nhan >> vmrss;
        }
        if (vmrss < 0) continue;   // kernel thread khong co VmRSS
        if (tienTrinhCanAn.count(name) > 0) continue;

        TienTrinhInfo t;
        t.pid = std::stoi(ten);
        t.ten = name;
        t.uid = uid;
        t.ramMb = std::round(vmrss / 1024.0 * 10) / 10;
        ds.push_back(t);
    }
    closedir(dir);

    std::sort(ds.begin(), ds.end(),
              [](const TienTrinhInfo& a, const TienTrinhInfo& b) { return a.ramMb > b.ramMb; });
    if (ds.size() > toiDa) ds.resize(toiDa);
    for (TienTrinhInfo& t : ds) t.user = tenUser(t.uid);   // chi tra ten user cho top N
    return ds;
}
