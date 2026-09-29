#include "tcp_server.h"
#include "shared.h"
#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

const int PORT = 9001;
const std::string SECRET_TOKEN = "abc123";

void chayTcpServer() {
    // === DÁN TOÀN BỘ NỘI DUNG int main() CŨ CỦA tcp_server.cpp VÀO ĐÂY ===
    // Bỏ 2 dòng khai báo PORT và SECRET_TOKEN cũ bên trong (đã khai báo ở trên rồi)
    // Bỏ các dòng "return -1;" và "return 0;" (hàm void không return giá trị)

    // Trong khối if (AuthToken đúng), SAU khi lấy được target và maxRam, thêm:
    // {
    //     std::lock_guard<std::mutex> lg(khoa);
    //     tenDangQuanLy = target;
    //     soKbMaxDungChung = (long)maxRam * 1024;
    // }
        // 1. Khởi tạo socket TCP
    int serverfd = socket(AF_INET, SOCK_STREAM, 0);
    if (serverfd < 0) {
        perror("Lỗi socket");
        return ;
    }

    int opt = 1;
    setsockopt(serverfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);

    // 2. Bind cổng 9001
    if (bind(serverfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("Lỗi bind");
        close(serverfd);
        return ;
    }

    // 3. Listen hàng đợi 5 kết nối
    if (listen(serverfd, 5) < 0) {
        perror("Lỗi listen");
        close(serverfd);
        return ;
    }

    std::cout << "Server đang chạy và lắng nghe ở cổng " << PORT << "..." << std::endl;

// 4. Vòng lặp phục vụ các client
    while (true) {
        sockaddr_in clientAddr{};
        socklen_t clientLen = sizeof(clientAddr); 
        std::cout << "Đang chờ client kết nối..." << std::endl;
        int clientFd = accept(serverfd, (struct sockaddr*)&clientAddr, &clientLen);
        if (clientFd < 0) {
            perror("Lỗi accept");
            continue;
        }

        // ==========================================
        // SỬA CƠ CHẾ NHẬN TCP GIỐNG BẢN WINDOWS
        // Đọc 1 lần, không chờ ký tự '\n'
        // ==========================================
        char buffer[4097];
        memset(buffer, 0, sizeof(buffer));
        ssize_t n = recv(clientFd, buffer, sizeof(buffer) - 1, 0);

        if (n <= 0) {
            // Client ngắt kết nối hoặc lỗi mạng
            close(clientFd);
            continue;
        }

        std::string jsonStr(buffer);
        std::cout << "Dữ liệu thô nhận được: " << jsonStr << std::endl;

        json phanHoi;

        // 5.Parse JSON & Kiểm tra AuthToken
        try {
            json j = json::parse(jsonStr);

            // Kiểm tra xem có trường AuthToken không và có đúng mật khẩu không
            if (j.contains("AuthToken") && j["AuthToken"] == SECRET_TOKEN) {
                std::string cmd = j.value("CommandType", "");
                std::string target = j.value("TargetProcess", "");
                int maxRam = j.value("MaxRamMB", 0);

                {
                    std::lock_guard<std::mutex> lg(khoa);
                    tenDangQuanLy = target;
                    soKbMaxDungChung = (long)maxRam * 1024;
                } 

                std::cout << "[HỢP LỆ] Đã nhận lệnh: " << cmd 
                          << " | Tiến trình: " << target 
                          << " | RAM tối đa: " << maxRam << "MB" << std::endl;

                phanHoi["status"] = "ok";
                phanHoi["message"] = "Policy applied";
            } else {
                std::cout << "[TỪ CHỐI] AuthToken không chính xác hoặc bị thiếu!" << std::endl;
                phanHoi["status"] = "error";
                phanHoi["message"] = "Invalid token";
            }
        } catch (const std::exception& e) {
            std::cout << "[LỖI CÚ PHÁP] Chuỗi không phải JSON hợp lệ: " << e.what() << std::endl;
            phanHoi["status"] = "error";
            phanHoi["message"] = "Malformed JSON";
        }

        // 6. Gửi trả kết quả JSON qua mạng
        std::string guiDi = phanHoi.dump() + "\n";
        send(clientFd, guiDi.c_str(), guiDi.length(), 0);

        // Đóng kết nối với client hiện tại
        close(clientFd);
    }

    close(serverfd);
    return ;

}