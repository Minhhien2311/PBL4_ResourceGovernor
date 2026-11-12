#ifndef SHARED_H
#define SHARED_H

#include <mutex>
#include <string>
#include <vector>

struct LuatRieng {
    long uid;                  // -1 = ap dung cho moi user
    std::string tenTienTrinh;
    long maxKb;                // 0 = khong gioi han RAM
    int maxCpu = 0;            // % cua MOT loi (100 = 1 loi), 0 = khong gioi han CPU
    bool thuHoiRam = false;    // true: tam dung xong thi ep RAM ra swap
};

extern std::mutex khoa;
extern std::vector<LuatRieng> danhSachLuat;

#endif