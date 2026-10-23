// #include "tcp_server.h"
// #include "shared.h"
// #include <iostream>
// #include <string>
// #include <cstring>
// #include <unistd.h>
// #include <sys/socket.h>
// #include <netinet/in.h>
// #include <nlohmann/json.hpp>

// #include <dirent.h>
// #include <fstream>
// #include <sstream>

// using json = nlohmann::json;

// const int PORT = 9001;
// const std::string SECRET_TOKEN = "abc123";

// double layRamTienTrinh(const std::string& pidStr) {
//     std::string duongDan = "/proc/" + pidStr + "/status";
//     std::ifstream file(duongDan);
//     if (!file.is_open()) return 0.0;
//     std::string dong;
//     while (std::getline(file, dong)) {
//         if (dong.substr(0, 6) == "VmRSS:") {
//             long kb = 0;
//             std::istringstream iss(dong.substr(6));
//             iss >> kb;
//             return kb / 1024.0; // Chuyển KB sang MB
//         }
//     }
//     return 0.0;
// }

// void chayTcpServer() {
//     // === DÁN TOÀN BỘ NỘI DUNG int main() CŨ CỦA tcp_server.cpp VÀO ĐÂY ===
//     // Bỏ 2 dòng khai báo PORT và SECRET_TOKEN cũ bên trong (đã khai báo ở trên rồi)
//     // Bỏ các dòng "return -1;" và "return 0;" (hàm void không return giá trị)

//     // Trong khối if (AuthToken đúng), SAU khi lấy được target và maxRam, thêm:
//     // {
//     //     std::lock_guard<std::mutex> lg(khoa);
//     //     tenDangQuanLy = target;
//     //     soKbMaxDungChung = (long)maxRam * 1024;
//     // }
//         // 1. Khởi tạo socket TCP
//     int serverfd = socket(AF_INET, SOCK_STREAM, 0);
//     if (serverfd < 0) {
//         perror("Lỗi socket");
//         return ;
//     }

//     int opt = 1;
//     setsockopt(serverfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

//     sockaddr_in addr{};
//     addr.sin_family = AF_INET;
//     addr.sin_addr.s_addr = INADDR_ANY;
//     addr.sin_port = htons(PORT);

//     // 2. Bind cổng 9001
//     if (bind(serverfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
//         perror("Lỗi bind");
//         close(serverfd);
//         return ;
//     }

//     // 3. Listen hàng đợi 5 kết nối
//     if (listen(serverfd, 5) < 0) {
//         perror("Lỗi listen");
//         close(serverfd);
//         return ;
//     }

//     std::cout << "Server đang chạy và lắng nghe ở cổng " << PORT << "..." << std::endl;

// // 4. Vòng lặp phục vụ các client
//     while (true) {
//         sockaddr_in clientAddr{};
//         socklen_t clientLen = sizeof(clientAddr); 
//         std::cout << "Đang chờ client kết nối..." << std::endl;
//         int clientFd = accept(serverfd, (struct sockaddr*)&clientAddr, &clientLen);
//         if (clientFd < 0) {
//             perror("Lỗi accept");
//             continue;
//         }

//         // ==========================================
//         // SỬA CƠ CHẾ NHẬN TCP GIỐNG BẢN WINDOWS
//         // Đọc 1 lần, không chờ ký tự '\n'
//         // ==========================================
//         char buffer[4097];
//         memset(buffer, 0, sizeof(buffer));
//         ssize_t n = recv(clientFd, buffer, sizeof(buffer) - 1, 0);

//         if (n <= 0) {
//             // Client ngắt kết nối hoặc lỗi mạng
//             close(clientFd);
//             continue;
//         }

//         std::string jsonStr(buffer);
//         std::cout << "Dữ liệu thô nhận được: " << jsonStr << std::endl;

//         json phanHoi;

//         // 5.Parse JSON & Kiểm tra AuthToken
//         try {
//             json j = json::parse(jsonStr);

//             // Kiểm tra xem có trường AuthToken không và có đúng mật khẩu không
//             if (j.contains("AuthToken") && j["AuthToken"] == SECRET_TOKEN) {
//                 std::string cmd = j.value("CommandType", "");
//                 std::string target = j.value("TargetProcess", "");
//                 int maxRam = j.value("MaxRamMB", 0);

//                 {
//                     std::lock_guard<std::mutex> lg(khoa);
//                     tenDangQuanLy = target;
//                     soKbMaxDungChung = (long)maxRam * 1024;
//                 } 

//                 std::cout << "[HỢP LỆ] Đã nhận lệnh: " << cmd 
//                           << " | Tiến trình: " << target 
//                           << " | RAM tối đa: " << maxRam << "MB" << std::endl;

//                 phanHoi["status"] = "ok";
//                 phanHoi["message"] = "Policy applied";
//             } else {
//                 std::cout << "[TỪ CHỐI] AuthToken không chính xác hoặc bị thiếu!" << std::endl;
//                 phanHoi["status"] = "error";
//                 phanHoi["message"] = "Invalid token";
//             }
//         } catch (const std::exception& e) {
//             std::cout << "[LỖI CÚ PHÁP] Chuỗi không phải JSON hợp lệ: " << e.what() << std::endl;
//             phanHoi["status"] = "error";
//             phanHoi["message"] = "Malformed JSON";
//         }

//         // 6. Gửi trả kết quả JSON qua mạng
//         std::string guiDi = phanHoi.dump() + "\n";
//         send(clientFd, guiDi.c_str(), guiDi.length(), 0);

//         // Đóng kết nối với client hiện tại
//         close(clientFd);
//     }

//     close(serverfd);
//     return ;

// }


#include "tcp_server.h"
#include "shared.h"
#include "metrics.h"
#include "config.h"
#include "logger.h"
#include <arpa/inet.h>
#include <cstring>
#include <dirent.h>
#include <fstream>
#include <iostream>
#include <netinet/in.h>
#include <nlohmann/json.hpp>
#include <pwd.h>
#include <sstream>
#include <string>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
#include "job.h"

using json = nlohmann::json;

const size_t MAX_LENH = 4096;
const int PORT = 9001;

// Ham phu tro: doc ten tien trinh va RAM tu /proc/[pid]/status
void layThongTinTienTrinh(const std::string& pidStr, std::string& ten, double& ramMB) {
    ten = "unknown";
    ramMB = 0.0;
    std::string duongDan = "/proc/" + pidStr + "/status";
    std::ifstream file(duongDan);
    if (!file.is_open()) return;

    std::string dong;
    while (std::getline(file, dong)) {
        if (dong.substr(0, 5) == "Name:") {
            std::istringstream iss(dong.substr(5));
            iss >> ten;
        }
        if (dong.substr(0, 6) == "VmRSS:") {
            long kb = 0;
            std::istringstream iss(dong.substr(6));
            iss >> kb;
            ramMB = kb / 1024.0;
        }
    }
}

void chayTcpServer() {
    const std::string SECRET_TOKEN = docCauHinh("Token", "abc123");

    int serverfd = socket(AF_INET, SOCK_STREAM, 0);
    if (serverfd < 0) {
        perror("Loi socket");
        return;
    }

    int opt = 1;
    setsockopt(serverfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);

    if (bind(serverfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("Loi bind");
        close(serverfd);
        return;
    }

    if (listen(serverfd, 5) < 0) {
        perror("Loi listen");
        close(serverfd);
        return;
    }

    std::cout << "TCP Server (Linux) dang lang nghe o cong " << PORT << "..." << std::endl;
    ghiNhatKy("Agent khoi dong, lang nghe cong " + std::to_string(PORT));

    while (true) {
        sockaddr_in clientAddr{};
        socklen_t clientLen = sizeof(clientAddr);
        int clientFd = accept(serverfd, (struct sockaddr*)&clientAddr, &clientLen);
        if (clientFd < 0) continue;

        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &clientAddr.sin_addr, ip, sizeof(ip));
        const std::string nguon = ip;

        // Timeout 5 giay moi lan cho du lieu
        struct timeval tv;
        tv.tv_sec = 5;
        tv.tv_usec = 0;
        setsockopt(clientFd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

        // Doc den khi du mot lenh: gap '\n', hoac da la JSON day du, hoac vuot gioi han
        std::string duLieu;
        char buffer[1024];
        bool quaDai = false;
        while (true) {
            ssize_t n = recv(clientFd, buffer, sizeof(buffer), 0);
            if (n <= 0) break;                           // client dong ket noi hoac het thoi gian cho
            duLieu.append(buffer, n);
            if (duLieu.size() > MAX_LENH) { quaDai = true; break; }
            if (duLieu.find('\n') != std::string::npos) break;
            if (json::accept(duLieu)) break;             // client khong gui '\n' nhung JSON da du
        }

        if (quaDai) {
            ghiNhatKy(nguon + " | TU CHOI | Lenh qua dai");
            std::string loi = "{\"status\":\"error\",\"message\":\"Message too large\"}\n";
            send(clientFd, loi.c_str(), loi.length(), 0);
            close(clientFd);
            continue;
        }
        if (duLieu.empty()) {
            close(clientFd);
            continue;
        }

        size_t pos = duLieu.find('\n');
        std::string jsonStr = (pos == std::string::npos) ? duLieu : duLieu.substr(0, pos);

        json phanHoi;
        phanHoi["NodeId"] = docCauHinh("NodeId", "server-a");

        try {
            json j = json::parse(jsonStr);

            if (j.contains("AuthToken") && j["AuthToken"] == SECRET_TOKEN) {
                std::string cmd = j.value("CommandType", "");

                if (cmd == "GetMetrics") {
                    SoLieu sl = laySoLieu();
                    phanHoi["status"] = "ok";
                    phanHoi["CPUUsagePercent"] = sl.cpuPhanTram;
                    phanHoi["TotalRAM"] = sl.tongRamMb;
                    phanHoi["UsedRAM"] = sl.daDungRamMb;
                    phanHoi["ProcessCount"] = sl.soTienTrinh;
                    ghiNhatKy(nguon + " | OK | GetMetrics");

                } else if (cmd == "UpdateOsPolicy") {
                    std::string target = j.value("TargetProcess", "");
                    std::string tenUser = j.value("TargetUser", "");
                    int maxRam = j.value("MaxRamMB", 0);

                    long uid = -1;
                    bool hopLe = (!target.empty() && maxRam > 0 && maxRam <= 1048576);

                    if (hopLe && !tenUser.empty()) {
                        struct passwd* p = getpwnam(tenUser.c_str());
                        if (p == nullptr) {
                            hopLe = false;
                        } else {
                            uid = p->pw_uid;
                        }
                    }

                    if (hopLe) {
                        bool thanhCong = true;
                        {
                            std::lock_guard<std::mutex> lg(khoa);
                            bool daCo = false;
                            for (LuatRieng& l : danhSachLuat) {
                                if (l.uid == uid && l.tenTienTrinh == target) {
                                    l.maxKb = (long)maxRam * 1024;
                                    daCo = true;
                                }
                            }
                            if (!daCo) {
                                if (danhSachLuat.size() >= 100) {
                                    thanhCong = false;
                                } else {
                                    LuatRieng moi;
                                    moi.uid = uid;
                                    moi.tenTienTrinh = target;
                                    moi.maxKb = (long)maxRam * 1024;
                                    danhSachLuat.push_back(moi);
                                }
                            }
                        }
                        if (thanhCong) {
                            std::string ten = tenUser.empty() ? std::string("moi user") : tenUser;
                            std::cout << "[HOP LE] UpdateOsPolicy | user: " << ten
                                      << " | process: " << target << " | " << maxRam << "MB" << std::endl;
                            ghiNhatKy(nguon + " | OK | Luat " + ten + "/" + target + "/" + std::to_string(maxRam) + "MB");
                            phanHoi["status"] = "ok";
                            phanHoi["message"] = "Policy applied";
                        } else {
                            std::cout << "[TU CHOI] Qua nhieu luat" << std::endl;
                            ghiNhatKy(nguon + " | TU CHOI | Qua nhieu luat");
                            phanHoi["status"] = "error";
                            phanHoi["message"] = "Too many rules";
                        }
                    } else {
                        std::cout << "[TU CHOI] Luat khong hop le hoac user khong ton tai" << std::endl;
                        ghiNhatKy(nguon + " | TU CHOI | Luat khong hop le hoac user khong ton tai: " + tenUser);
                        phanHoi["status"] = "error";
                        phanHoi["message"] = "Invalid policy or unknown user";
                    }
                } else if (cmd == "SubmitJob") {
                    phanHoi.update(xuLySubmitJob(j, nguon));
                } else if (cmd == "GetJobStatus") {
                    phanHoi.update(xuLyGetJobStatus(j));
                } else if (cmd == "CancelJob") {
                    phanHoi.update(xuLyCancelJob(j, nguon));
                } else {
                    std::cout << "[TU CHOI] Lenh khong ho tro: " << cmd << std::endl;
                    ghiNhatKy(nguon + " | TU CHOI | Lenh khong ho tro: " + cmd);
                    phanHoi["status"] = "error";
                    phanHoi["message"] = "Unknown command";
                }

            } else {
                std::cout << "[TU CHOI] AuthToken khong chinh xac hoac bi thieu!" << std::endl;
                ghiNhatKy(nguon + " | TU CHOI | Sai token");
                phanHoi["status"] = "error";
                phanHoi["message"] = "Invalid token";
            }
        } catch (const std::exception& e) {
            std::cout << "[LOI CU PHAP] " << e.what() << std::endl;
            ghiNhatKy(nguon + " | TU CHOI | Malformed JSON");
            phanHoi["status"] = "error";
            phanHoi["message"] = "Malformed JSON";
        }

        std::string guiDi = phanHoi.dump() + "\n";
        send(clientFd, guiDi.c_str(), guiDi.length(), 0);
        close(clientFd);
    }
    close(serverfd);
}