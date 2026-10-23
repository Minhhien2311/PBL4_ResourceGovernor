#include "api_server.h"
#include "jobstore.h"
#include "logger.h"
#include "poller.h"
#include "registry.h"
#include "tracker.h"
#include <cstdio>
#include <iomanip>
#include <iostream>
#include <string>
#include <thread>
#include <unistd.h>

int main(int argc, char** argv) {
    std::string tenFile = (argc > 1) ? argv[1] : "servers.conf";
    if (!napDanhSach(tenFile)) {
        std::cerr << "Khong doc duoc danh sach server tu " << tenFile
                  << " (file khong ton tai o thu muc hien tai, hoac khong co dong nao dung dinh dang ten,ip,cong,token)"
                  << std::endl;
        return 1;
    }
    ghiNhatKy("Controller khoi dong, " + std::to_string(layBanSao().size()) + " server");

    std::thread(chayPoller, 5).detach();
    std::thread(chayTracker, 3).detach();
    std::thread(chayApiServer).detach();

    while (true) {
        std::cout << "\033[H\033[2J";
        std::cout << std::left << std::setw(12) << "SERVER" << std::setw(16) << "IP"
                  << std::setw(14) << "TRANG THAI" << std::setw(8) << "CPU%"
                  << std::setw(16) << "RAM dung/tong" << std::setw(7) << "PROC"
                  << "GHI CHU" << std::endl;
        for (const ServerInfo& s : layBanSao()) {
            std::string st = trangThai(s);
            bool coSo = (st == "ONLINE" || st == "DEGRADED");
            char cpu[16];
            std::snprintf(cpu, sizeof(cpu), "%.1f", s.cpu);
            std::string ram = coSo ? std::to_string(s.daDungRamMb) + "/" + std::to_string(s.tongRamMb) + " MB" : "-";
            std::cout << std::left << std::setw(12) << s.ten << std::setw(16) << s.ip
                      << std::setw(14) << st << std::setw(8) << (coSo ? std::string(cpu) : std::string("-"))
                      << std::setw(16) << ram << std::setw(7) << (coSo ? std::to_string(s.soTienTrinh) : std::string("-"))
                      << s.loiCuoi << std::endl;
        }

        std::cout << "\n" << std::left << std::setw(10) << "JOB" << std::setw(14) << "LOAI"
                  << std::setw(8) << "USER" << std::setw(12) << "TRANG THAI"
                  << std::setw(11) << "SERVER" << "LY DO" << std::endl;
        for (const JobCtl& j : layTatCa()) {
            std::string lyDo = j.reason.substr(0, 70);
            std::cout << std::left << std::setw(10) << j.id << std::setw(14) << j.loai
                      << std::setw(8) << j.user << std::setw(12) << j.state
                      << std::setw(11) << j.server << lyDo << std::endl;
        }
        sleep(2);
    }
}