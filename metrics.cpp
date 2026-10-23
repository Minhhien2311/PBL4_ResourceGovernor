#include "metrics.h"
#include <cctype>
#include <dirent.h>
#include <fstream>
#include <sstream>
#include <string>
#include <unistd.h>

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