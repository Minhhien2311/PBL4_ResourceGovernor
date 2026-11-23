#include "logger.h"
#include <ctime>
#include <fstream>
#include <mutex>

void ghiNhatKy(const std::string& noiDung) {
    static std::mutex m;
    std::lock_guard<std::mutex> lg(m);

    std::ofstream f("agent.log", std::ios::app);   // mo file de GHI NOI THEM vao cuoi
    time_t now = time(nullptr);
    struct tm tmv;
    localtime_r(&now, &tmv);
    char buf[32];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tmv);
    f << buf << " | " << noiDung << std::endl;
}