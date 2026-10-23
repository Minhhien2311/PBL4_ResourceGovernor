#include "config.h"
#include <fstream>

std::string docCauHinh(const std::string& khoa, const std::string& macDinh) {
    std::ifstream file("controller.conf");
    std::string dong;
    while (std::getline(file, dong)) {
        size_t pos = dong.find('=');
        if (pos == std::string::npos) continue;
        if (dong.substr(0, pos) == khoa) return dong.substr(pos + 1);
    }
    return macDinh;
}