#include "metrics.h"
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

static bool docCpu(long& tong, long& rang) {
    std::ifstream file("/proc/stat");
    std::string dong;
    if (!std::getline(file, dong)) return false;
    std::istringstream iss(dong);
    std::string nhan;
    long user, nice, system, idle, iowait, irq, softirq, steal;
    iss >> nhan >> user >> nice >> system >> idle >> iowait >> irq >> softirq >> steal;
    tong = user + nice + system + idle + iowait + irq + softirq + steal;
    rang = idle + iowait;
    return true;
}

SoLieu laySoLieu() {
    SoLieu s{0, 0, 0, 0};

    // CPU: do hai lan cach nhau 0.5 giay
    long tong1, rang1, tong2, rang2;
    if (docCpu(tong1, rang1)) {
        usleep(500000);
        if (docCpu(tong2, rang2) && tong2 > tong1) {
            s.cpuPhanTram = 100.0 * (double)((tong2 - tong1) - (rang2 - rang1)) / (double)(tong2 - tong1);;
        }
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