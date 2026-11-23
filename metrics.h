#ifndef METRICS_H
#define METRICS_H
#include <string>
#include <vector>

struct SoLieu {
    double cpuPhanTram;
    long tongRamMb;
    long daDungRamMb;
    int soTienTrinh;
};

SoLieu laySoLieu();

struct TienTrinhInfo {
    int pid;
    std::string ten;
    std::string user;
    long uid;
    double ramMb;
};

std::vector<TienTrinhInfo> layTienTrinh(size_t toiDa);   // top tien trinh dung nhieu RAM nhat

#endif