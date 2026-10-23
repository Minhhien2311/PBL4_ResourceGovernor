#ifndef SHARED_H
#define SHARED_H

#include <mutex>
#include <string>
#include <vector>

struct LuatRieng {
    long uid;                  // -1 = ap dung cho moi user
    std::string tenTienTrinh;
    long maxKb;
};

extern std::mutex khoa;
extern std::vector<LuatRieng> danhSachLuat;

#endif