#include "config.h"
#include <fstream>

static std::string cat(const std::string& s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos) return "";
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

std::string docCauHinh(const std::string& khoa, const std::string& macDinh) {
    std::ifstream file("agent.conf");
    std::string dong;
    while (std::getline(file, dong)) {
        size_t pos = dong.find('=');
        if (pos == std::string::npos) continue;
        std::string k = cat(dong.substr(0, pos));
        std::string v = cat(dong.substr(pos + 1));
        if (k == khoa) return v;
    }
    return macDinh;
}