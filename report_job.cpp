#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <csignal>

// Cờ để hỗ trợ dừng mềm nếu nhận tín hiệu SIGTERM từ Agent
volatile sig_atomic_t dangChay = 1;

void xuLyTinHieu(int sig) {
    if (sig == SIGTERM || sig == SIGINT) {
        dangChay = 0;
    }
}

int main(int argc, char* argv[]) {
    // Đăng ký nhận tín hiệu để Agent có thể CancelJob một cách an toàn
    struct sigaction sa;
    std::memset(&sa, 0, sizeof(sa));
    sa.sa_handler = xuLyTinHieu;
    sigaction(SIGTERM, &sa, nullptr);
    sigaction(SIGINT, &sa, nullptr);

    // Mặc định: Chạy 20 giây và chiếm dụng 100 MB RAM
    int thoiGianChay = 20;
    size_t ramMb = 100;

    // Đọc tham số từ dòng lệnh do Agent truyền vào qua execv
    if (argc > 1) {
        thoiGianChay = std::atoi(argv[1]);
        if (thoiGianChay <= 0) thoiGianChay = 20;
    }
    if (argc > 2) {
        long m = std::atol(argv[2]);
        if (m > 0) ramMb = static_cast<size_t>(m);
    }

    // Cấp phát bộ nhớ RAM theo yêu cầu
    size_t tongBytes = ramMb * 1024 * 1024;
    std::vector<char> vungNho;

    try {
        vungNho.resize(tongBytes);
        // Quan trọng: Phải ghi đè dữ liệu lên từng trang bộ nhớ (Page touch: 4096 bytes)
        // để Linux Kernel thực sự ánh xạ RAM vật lý (tăng Resident Set Size - RSS)
        for (size_t i = 0; i < tongBytes; i += 4096) {
            vungNho[i] = static_cast<char>(i % 255);
        }
    } catch (const std::bad_alloc& e) {
        return 1; // Thoát lỗi nếu máy không đủ RAM cấp phát
    }

    // Giữ tiến trình chạy trong đúng khoảng thời gian được yêu cầu
    int demGiay = 0;
    while (dangChay && demGiay < thoiGianChay) {
        sleep(1);
        demGiay++;

        // Chạm nhẹ vào bộ nhớ định kỳ để đảm bảo RAM không bị hoán đổi (swap out)
        if (!vungNho.empty()) {
            vungNho[(demGiay * 4096) % tongBytes] += 1;
        }
    }

    // Trả về 0 khi hoàn thành bình thường (ExitCode = 0)
    return 0;
}