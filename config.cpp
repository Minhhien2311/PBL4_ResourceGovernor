#include "config.h"
#include <fstream>

std::string docCauHinh(const std::string& khoa, const std::string& macDinh) {
    std::ifstream file("agent.conf");
    std::string dong;
    while (std::getline(file, dong)) {
        size_t pos = dong.find('=');
        if (pos == std::string::npos) continue;
        std::string k = dong.substr(0, pos);
        std::string v = dong.substr(pos + 1);
        if (k == khoa) return v;
    }
    return macDinh;
}