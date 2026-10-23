#ifndef SCHEDULER_H
#define SCHEDULER_H
#include <string>
#include <vector>

struct KetQuaChon {
    int idx = -1;
    std::string lyDo;
};

KetQuaChon chonServer(long memoryMb, const std::vector<std::string>& loaiTru);
bool giaoJob(const std::string& id);   // giao job PENDING cho server phu hop, true neu giao duoc
#endif