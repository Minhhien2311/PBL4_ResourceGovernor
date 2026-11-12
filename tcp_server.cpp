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

/*
#include "tcp_server.h"
#include "shared.h"
#include "metrics.h"
#include "config.h"
#include "logger.h"
#include "job.h"

#include <arpa/inet.h>
#include <cerrno>
#include <climits>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <mutex>

#include <dirent.h>
#include <netinet/in.h>
#include <pwd.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>

#include <nlohmann/json.hpp>

using json = nlohmann::json;

const size_t MAX_LENH = 4096;
const int PORT = 9001;

// ============================================================
// Tinh HMAC-SHA256, tra ve chu ky hex 64 ky tu
// ============================================================
static bool tinhHmacSha256(
    const std::string& payload,
    const std::string& secret,
    std::string& chuKyHex
) {
    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int digestLen = 0;

    unsigned char* ketQua = HMAC(
        EVP_sha256(),
        secret.data(),
        static_cast<int>(secret.size()),
        reinterpret_cast<const unsigned char*>(payload.data()),
        payload.size(),
        digest,
        &digestLen
    );

    if (ketQua == nullptr || digestLen != 32) {
        return false;
    }

    std::ostringstream ss;
    ss << std::hex << std::setfill('0');

    for (unsigned int i = 0; i < digestLen; ++i) {
        ss << std::setw(2)
           << static_cast<unsigned int>(digest[i]);
    }

    chuKyHex = ss.str();
    return true;
}

// ============================================================
// So sanh chu ky theo thoi gian hang dinh
// ============================================================
static bool chuKyBangNhauHangDinh(
    const std::string& a,
    const std::string& b
) {
    if (a.size() != b.size()) {
        return false;
    }

    return CRYPTO_memcmp(a.data(), b.data(), a.size()) == 0;
}

// ============================================================
// Doc ten tien trinh va RAM tu /proc/[pid]/status
// ============================================================
void layThongTinTienTrinh(
    const std::string& pidStr,
    std::string& ten,
    double& ramMB
) {
    ten = "unknown";
    ramMB = 0.0;

    std::string duongDan = "/proc/" + pidStr + "/status";
    std::ifstream file(duongDan);

    if (!file.is_open()) {
        return;
    }

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

// ============================================================
// Lay ten user tu UID
// ============================================================
static std::string tenTuUid(long uid) {
    struct passwd pw, *kq = nullptr;
    char buf[1024];

    if (getpwuid_r(
            static_cast<uid_t>(uid),
            &pw,
            buf,
            sizeof(buf),
            &kq
        ) == 0 && kq) {
        return pw.pw_name;
    }

    return std::to_string(uid);
}

// ============================================================
// TCP SERVER
//
// Ho tro hai giao thuc:
//
// 1. HMAC:
//    {
//      "Payload": "{\"CommandType\":\"GetMetrics\",...}",
//      "Signature": "..."
//    }
//
// 2. Web Admin:
//    {
//      "AuthToken": "...",
//      "CommandType": "GetMetrics",
//      ...
//    }
//
// Sau khi xac thuc, ca hai deu dua payload vao bien j
// va dung chung khoi xu ly lenh ben duoi.
// ============================================================
void chayTcpServer() {
    const std::string SECRET_TOKEN =
        docCauHinh("Token", "abc123");

    int serverfd = socket(AF_INET, SOCK_STREAM, 0);

    if (serverfd < 0) {
        perror("Loi socket");
        return;
    }

    int opt = 1;
    setsockopt(
        serverfd,
        SOL_SOCKET,
        SO_REUSEADDR,
        &opt,
        sizeof(opt)
    );

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);

    if (bind(
            serverfd,
            reinterpret_cast<struct sockaddr*>(&addr),
            sizeof(addr)
        ) < 0) {
        perror("Loi bind");
        close(serverfd);
        return;
    }

    if (listen(serverfd, 5) < 0) {
        perror("Loi listen");
        close(serverfd);
        return;
    }

    std::cout
        << "TCP Server (Linux) dang lang nghe o cong "
        << PORT << "..."
        << std::endl;

    ghiNhatKy(
        "Agent khoi dong, lang nghe cong "
        + std::to_string(PORT)
    );

    while (true) {
        sockaddr_in clientAddr{};
        socklen_t clientLen = sizeof(clientAddr);

        int clientFd = accept(
            serverfd,
            reinterpret_cast<struct sockaddr*>(&clientAddr),
            &clientLen
        );

        if (clientFd < 0) {
            if (errno == EINTR) {
                continue;
            }

            perror("Loi accept");
            continue;
        }

        char ip[INET_ADDRSTRLEN]{};

        inet_ntop(
            AF_INET,
            &clientAddr.sin_addr,
            ip,
            sizeof(ip)
        );

        const std::string nguon = ip;

        // Timeout doc du lieu: 5 giay
        struct timeval tv{};
        tv.tv_sec = 5;
        tv.tv_usec = 0;

        setsockopt(
            clientFd,
            SOL_SOCKET,
            SO_RCVTIMEO,
            &tv,
            sizeof(tv)
        );

        // ----------------------------------------------------
        // Doc request TCP
        // Ket thuc khi gap newline hoac nhan du JSON hoan chinh
        // ----------------------------------------------------
        std::string duLieu;
        char buffer[1024];

        bool quaDai = false;
        bool hetThoiGian = false;
        bool loiDoc = false;

        while (true) {
            ssize_t n = recv(
                clientFd,
                buffer,
                sizeof(buffer),
                0
            );

            if (n == 0) {
                // Client dong ket noi
                break;
            }

            if (n < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    hetThoiGian = true;
                } else if (errno != EINTR) {
                    loiDoc = true;
                }

                if (errno == EINTR) {
                    continue;
                }

                break;
            }

            duLieu.append(buffer, static_cast<size_t>(n));

            if (duLieu.size() > MAX_LENH) {
                quaDai = true;
                break;
            }

            // Neu request co newline thi dung tai newline
            if (duLieu.find('\n') != std::string::npos) {
                break;
            }

            // Neu JSON da hoan chinh thi khong can cho newline
            if (json::accept(duLieu)) {
                break;
            }
        }

        if (quaDai) {
            ghiNhatKy(
                nguon + " | TU CHOI | Lenh qua dai"
            );

            const std::string loi =
                "{\"status\":\"error\","
                "\"message\":\"Message too large\"}\n";

            send(clientFd, loi.c_str(), loi.size(), 0);
            close(clientFd);
            continue;
        }

        if (duLieu.empty()) {
            if (hetThoiGian) {
                ghiNhatKy(
                    nguon + " | LOI | Timeout khi doc request"
                );
            } else if (loiDoc) {
                ghiNhatKy(
                    nguon + " | LOI | Loi doc request"
                );
            }

            close(clientFd);
            continue;
        }

        // Lay dong JSON dau tien
        size_t pos = duLieu.find('\n');

        std::string jsonStr =
            (pos == std::string::npos)
                ? duLieu
                : duLieu.substr(0, pos);

        json phanHoi;
        phanHoi["NodeId"] =
            docCauHinh("NodeId", "server-a");

        try {
            json outer = json::parse(jsonStr);

            // DEBUG: in cac key cua request
            if (outer.is_object()) {
                std::cout
                    << "[DEBUG TCP] Request tu "
                    << nguon << " | Keys:";

                for (auto it = outer.begin();
                     it != outer.end();
                     ++it) {
                    std::cout << " " << it.key();
                }

                std::cout << std::endl;
            } else {
                std::cout
                    << "[DEBUG TCP] Request khong phai JSON object tu "
                    << nguon << std::endl;
            }

            json j;

            bool xacThucThanhCong = false;
            std::string loaiGiaoThuc;
            std::string lyDoTuChoi =
                "Missing Payload or Signature";

            // =================================================
            // NHANH 1: REQUEST HMAC
            // =================================================
            if (
                outer.is_object()
                && outer.contains("Payload")
                && outer["Payload"].is_string()
                && outer.contains("Signature")
                && outer["Signature"].is_string()
            ) {
                loaiGiaoThuc = "HMAC";

                const std::string payloadStr =
                    outer["Payload"].get<std::string>();

                const std::string clientSignature =
                    outer["Signature"].get<std::string>();

                std::string serverSignature;

                bool tinhDuocHmac = tinhHmacSha256(
                    payloadStr,
                    SECRET_TOKEN,
                    serverSignature
                );

                bool chuKyHopLe =
                    tinhDuocHmac
                    && chuKyBangNhauHangDinh(
                        clientSignature,
                        serverSignature
                    );

                if (!tinhDuocHmac) {
                    lyDoTuChoi = "HMAC calculation failed";
                } else if (!chuKyHopLe) {
                    lyDoTuChoi = "Invalid HMAC signature";
                }

                if (chuKyHopLe) {
                    try {
                        j = json::parse(payloadStr);

                        // Timestamp phai nam trong Payload
                        if (
                            j.is_object()
                            && j.contains("Timestamp")
                            && j["Timestamp"].is_number_integer()
                        ) {
                            const long long clientTime =
                                j["Timestamp"].get<long long>();

                            const long long serverTime =
                                static_cast<long long>(
                                    std::time(nullptr)
                                );

                            if (
                                clientTime > 0
                                && std::llabs(
                                    serverTime - clientTime
                                ) <= 5
                            ) {
                                xacThucThanhCong = true;
                            } else {
                                lyDoTuChoi = "Packet expired";
                            }
                        } else {
                            lyDoTuChoi =
                                "Missing or invalid Timestamp";
                        }
                    } catch (const std::exception&) {
                        lyDoTuChoi = "Malformed Payload JSON";
                    }
                }
            }

            // =================================================
            // NHANH 2: REQUEST TRUC TIEP TU WEB ADMIN
            // =================================================
            else if (
                outer.is_object()
                && outer.contains("AuthToken")
                && outer["AuthToken"].is_string()
                && outer.contains("CommandType")
                && outer["CommandType"].is_string()
            ) {
                loaiGiaoThuc = "AuthToken";

                const std::string clientToken =
                    outer["AuthToken"].get<std::string>();

                // So sanh token theo thoi gian hang dinh
                bool tokenHopLe =
                    chuKyBangNhauHangDinh(
                        clientToken,
                        SECRET_TOKEN
                    );

                if (tokenHopLe) {
                    // Request truc tiep:
                    // JSON goc chinh la payload xu ly lenh
                    j = outer;
                    xacThucThanhCong = true;
                } else {
                    lyDoTuChoi = "Invalid AuthToken";
                }
            }

            // =================================================
            // NHANH 3: KHONG DUNG DINH DANG HO TRO
            // =================================================
            else {
                lyDoTuChoi =
                    "Unsupported request format";
            }

            // =================================================
            // XU LY LENH CHUNG SAU KHI XAC THUC
            // =================================================
            if (xacThucThanhCong) {
                std::string cmd =
                    j.value("CommandType", "");

                std::cout
                    << "[DEBUG TCP] Xac thuc OK"
                    << " | Protocol: " << loaiGiaoThuc
                    << " | CommandType: " << cmd
                    << std::endl;

                // -------------------------------------------------
                // GET METRICS
                // -------------------------------------------------
                if (cmd == "GetMetrics") {
                    SoLieu sl = laySoLieu();

                    phanHoi["status"] = "ok";
                    phanHoi["CPUUsagePercent"] =
                        sl.cpuPhanTram;
                    phanHoi["TotalRAM"] = sl.tongRamMb;
                    phanHoi["UsedRAM"] = sl.daDungRamMb;
                    phanHoi["ProcessCount"] = sl.soTienTrinh;

                    std::vector<TienTrinhInfo> dsTT =
                        layTienTrinh(30);

                    json mang = json::array();

                    for (const TienTrinhInfo& t : dsTT) {
                        json o;

                        o["name"] = t.ten;
                        o["pid"] = t.pid;
                        o["user"] = t.user;
                        o["ramMB"] = t.ramMb;

                        mang.push_back(o);
                    }

                    phanHoi["Processes"] = mang;

                    ghiNhatKy(
                        nguon + " | OK | GetMetrics"
                    );
                }

                // -------------------------------------------------
                // UPDATE OS POLICY
                // -------------------------------------------------
                else if (cmd == "UpdateOsPolicy") {
                    std::string target =
                        j.value("TargetProcess", "");

                    std::string tenUser =
                        j.value("TargetUser", "");

                    int maxRam =
                        j.value("MaxRamMB", 0);

                    long uid = -1;

                    bool hopLe =
                        !target.empty()
                        && maxRam > 0
                        && maxRam <= 1048576;

                    if (hopLe && !tenUser.empty()) {
                        struct passwd* p =
                            getpwnam(tenUser.c_str());

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
                                if (
                                    l.uid == uid
                                    && l.tenTienTrinh == target
                                ) {
                                    l.maxKb =
                                        static_cast<long>(maxRam)
                                        * 1024;

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
                                    moi.maxKb =
                                        static_cast<long>(maxRam)
                                        * 1024;

                                    danhSachLuat.push_back(moi);
                                }
                            }
                        }

                        if (thanhCong) {
                            std::string ten =
                                tenUser.empty()
                                    ? std::string("moi user")
                                    : tenUser;

                            std::cout
                                << "[HOP LE] UpdateOsPolicy"
                                << " | user: " << ten
                                << " | process: " << target
                                << " | " << maxRam << "MB"
                                << std::endl;

                            ghiNhatKy(
                                nguon + " | OK | Luat "
                                + ten + "/" + target + "/"
                                + std::to_string(maxRam) + "MB"
                            );

                            phanHoi["status"] = "ok";
                            phanHoi["message"] = "Policy applied";
                        } else {
                            std::cout
                                << "[TU CHOI] Qua nhieu luat"
                                << std::endl;

                            ghiNhatKy(
                                nguon
                                + " | TU CHOI | Qua nhieu luat"
                            );

                            phanHoi["status"] = "error";
                            phanHoi["message"] = "Too many rules";
                        }
                    } else {
                        std::cout
                            << "[TU CHOI] Luat khong hop le "
                            << "hoac user khong ton tai"
                            << std::endl;

                        ghiNhatKy(
                            nguon
                            + " | TU CHOI | Luat khong hop le "
                            "hoac user khong ton tai: "
                            + tenUser
                        );

                        phanHoi["status"] = "error";
                        phanHoi["message"] =
                            "Invalid policy or unknown user";
                    }
                }

                // -------------------------------------------------
                // GET OS POLICIES
                // -------------------------------------------------
                else if (cmd == "GetOsPolicies") {
                    json arr = json::array();

                    {
                        std::lock_guard<std::mutex> lg(khoa);

                        for (const LuatRieng& l : danhSachLuat) {
                            json o;

                            o["TargetUser"] =
                                (l.uid == -1)
                                    ? std::string("")
                                    : tenTuUid(l.uid);

                            o["TargetProcess"] =
                                l.tenTienTrinh;

                            o["MaxRamMB"] =
                                l.maxKb / 1024;

                            arr.push_back(o);
                        }
                    }

                    phanHoi["status"] = "ok";
                    phanHoi["Policies"] = arr;

                    ghiNhatKy(
                        nguon + " | OK | GetOsPolicies"
                    );
                }

                // -------------------------------------------------
                // REMOVE OS POLICY
                // -------------------------------------------------
                else if (cmd == "RemoveOsPolicy") {
                    std::string target =
                        j.value("TargetProcess", "");

                    std::string tenUser =
                        j.value("TargetUser", "");

                    long uid = -1;

                    bool hopLe = !target.empty();

                    if (hopLe && !tenUser.empty()) {
                        struct passwd* p =
                            getpwnam(tenUser.c_str());

                        if (p == nullptr) {
                            hopLe = false;
                        } else {
                            uid = p->pw_uid;
                        }
                    }

                    if (!hopLe) {
                        phanHoi["status"] = "error";
                        phanHoi["message"] =
                            "Invalid policy or unknown user";
                    } else {
                        size_t xoa = 0;

                        {
                            std::lock_guard<std::mutex> lg(khoa);

                            for (
                                auto it = danhSachLuat.begin();
                                it != danhSachLuat.end();
                            ) {
                                if (
                                    it->uid == uid
                                    && it->tenTienTrinh == target
                                ) {
                                    it = danhSachLuat.erase(it);
                                    xoa++;
                                } else {
                                    ++it;
                                }
                            }
                        }

                        ghiNhatKy(
                            nguon + " | OK | Go luat "
                            + (tenUser.empty()
                                ? std::string("moi user")
                                : tenUser)
                            + "/" + target
                        );

                        phanHoi["status"] =
                            xoa ? "ok" : "error";

                        phanHoi["message"] =
                            xoa
                                ? "Policy removed"
                                : "Policy not found";
                    }
                }

                // -------------------------------------------------
                // SUBMIT JOB
                // -------------------------------------------------
                else if (cmd == "SubmitJob") {
                    phanHoi.update(
                        xuLySubmitJob(j, nguon)
                    );
                }

                // -------------------------------------------------
                // GET JOB STATUS
                // -------------------------------------------------
                else if (cmd == "GetJobStatus") {
                    phanHoi.update(
                        xuLyGetJobStatus(j)
                    );
                }

                // -------------------------------------------------
                // CANCEL JOB
                // -------------------------------------------------
                else if (cmd == "CancelJob") {
                    phanHoi.update(
                        xuLyCancelJob(j, nguon)
                    );
                }

                // -------------------------------------------------
                // UNKNOWN COMMAND
                // -------------------------------------------------
                else {
                    std::cout
                        << "[TU CHOI] Lenh khong ho tro: "
                        << cmd << std::endl;

                    ghiNhatKy(
                        nguon + " | TU CHOI | Lenh khong ho tro: "
                        + cmd
                    );

                    phanHoi["status"] = "error";
                    phanHoi["message"] = "Unknown command";
                }
            }

            // =================================================
            // REQUEST KHONG QUA XAC THUC
            // =================================================
            else {
                std::cout
                    << "[TU CHOI] Xac thuc khong hop le: "
                    << lyDoTuChoi
                    << " | Protocol: " << loaiGiaoThuc
                    << " | IP: " << nguon
                    << std::endl;

                ghiNhatKy(
                    nguon + " | TU CHOI | " + lyDoTuChoi
                );

                phanHoi["status"] = "error";
                phanHoi["message"] = lyDoTuChoi;
            }
        }
        catch (const std::exception& e) {
            std::cout
                << "[LOI CU PHAP] "
                << e.what()
                << std::endl;

            ghiNhatKy(
                nguon + " | TU CHOI | Malformed JSON"
            );

            phanHoi["status"] = "error";
            phanHoi["message"] = "Malformed JSON";
        }

        // ----------------------------------------------------
        // Gui response JSON
        // ----------------------------------------------------
        std::string guiDi = phanHoi.dump() + "\n";

        size_t daGui = 0;

        while (daGui < guiDi.size()) {
            ssize_t n = send(
                clientFd,
                guiDi.data() + daGui,
                guiDi.size() - daGui,
                0
            );

            if (n < 0) {
                if (errno == EINTR) {
                    continue;
                }
                break;
            }

            if (n == 0) {
                break;
            }

            daGui += static_cast<size_t>(n);
        }

        close(clientFd);
    }

    close(serverfd);
}

*/


#include "tcp_server.h"
#include "shared.h"
#include "metrics.h"
#include "config.h"
#include "logger.h"
#include "job.h"
#include <arpa/inet.h>
#include <cerrno>
#include <climits>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <mutex>
#include <dirent.h>
#include <netinet/in.h>
#include <pwd.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <nlohmann/json.hpp>
#include <iterator>

using json = nlohmann::json;

const size_t MAX_LENH = 8192;
const int PORT = 9001;

// ============================================================
// Tinh HMAC-SHA256, tra ve chu ky hex 64 ky tu
// ============================================================
static bool tinhHmacSha256(
    const std::string& payload,
    const std::string& secret,
    std::string& chuKyHex
) {
    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int digestLen = 0;

    unsigned char* ketQua = HMAC(
        EVP_sha256(),
        secret.data(),
        static_cast<int>(secret.size()),
        reinterpret_cast<const unsigned char*>(payload.data()),
        payload.size(),
        digest,
        &digestLen
    );

    if (ketQua == nullptr || digestLen != 32) {
        return false;
    }

    std::ostringstream ss;
    ss << std::hex << std::setfill('0');

    for (unsigned int i = 0; i < digestLen; ++i) {
        ss << std::setw(2)
           << static_cast<unsigned int>(digest[i]);
    }

    chuKyHex = ss.str();
    return true;
}

// ============================================================
// So sanh chu ky theo thoi gian hang dinh
// ============================================================
static bool chuKyBangNhauHangDinh(
    const std::string& a,
    const std::string& b
) {
    if (a.size() != b.size()) {
        return false;
    }

    return CRYPTO_memcmp(a.data(), b.data(), a.size()) == 0;
}

// ============================================================
// Doc ten tien trinh va RAM tu /proc/[pid]/status
// ============================================================
void layThongTinTienTrinh(
    const std::string& pidStr,
    std::string& ten,
    double& ramMB
) {
    ten = "unknown";
    ramMB = 0.0;

    std::string duongDan = "/proc/" + pidStr + "/status";
    std::ifstream file(duongDan);

    if (!file.is_open()) {
        return;
    }

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

// ============================================================
// Lay ten user tu UID
// ============================================================
static std::string tenTuUid(long uid) {
    struct passwd pw, *kq = nullptr;
    char buf[1024];

    if (getpwuid_r(static_cast<uid_t>(uid), &pw, buf, sizeof(buf), &kq) == 0 && kq) {
        return pw.pw_name;
    }

    return std::to_string(uid);
}

// Hàm mã hóa Base64 dùng OpenSSL
static std::string base64Encode(const std::string& in) {
    std::string out;
    out.resize(4 * ((in.size() + 2) / 3));
    int len = EVP_EncodeBlock((unsigned char*)out.data(), (const unsigned char*)in.data(), in.size());
    out.resize(len);
    return out;
}

// ============================================================
// TCP SERVER
//
// Ho tro hai giao thuc:
//
// 1. HMAC:
//    {
//      "Payload": "{\"CommandType\":\"GetMetrics\",...}",
//      "Signature": "..."
//    }
//
// 2. Web Admin:
//    {
//      "AuthToken": "...",
//      "CommandType": "GetMetrics",
//      ...
//    }
//
// Sau khi xac thuc, ca hai deu dua payload vao bien j
// va dung chung khoi xu ly lenh ben duoi.
//
// [MOI] Dat RequireHmac=1 trong agent.conf de tu choi giao thuc 2.
// ============================================================
void chayTcpServer() {
    const std::string SECRET_TOKEN = docCauHinh("Token", "abc123");

    // [MOI] Cong tac: chi nhan goi tin ky HMAC (mac dinh tat)
    const bool CHI_HMAC = (docCauHinh("RequireHmac", "0") == "1");

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

    if (bind(serverfd, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) < 0) {
        perror("Loi bind");
        close(serverfd);
        return;
    }

    if (listen(serverfd, 5) < 0) {
        perror("Loi listen");
        close(serverfd);
        return;
    }

    std::cout
        << "TCP Server (Linux) dang lang nghe o cong "
        << PORT << "..."
        << std::endl;

    ghiNhatKy(
        "Agent khoi dong, lang nghe cong "
        + std::to_string(PORT)
        + (CHI_HMAC ? " | chi nhan HMAC" : " | nhan HMAC va AuthToken")
    );

    while (true) {
        sockaddr_in clientAddr{};
        socklen_t clientLen = sizeof(clientAddr);

        int clientFd = accept(serverfd, reinterpret_cast<struct sockaddr*>(&clientAddr), &clientLen);

        if (clientFd < 0) {
            if (errno == EINTR) {
                continue;
            }

            perror("Loi accept");
            continue;
        }

        char ip[INET_ADDRSTRLEN]{};

        inet_ntop(AF_INET, &clientAddr.sin_addr, ip, sizeof(ip));

        const std::string nguon = ip;

        // Timeout doc du lieu: 5 giay
        struct timeval tv{};
        tv.tv_sec = 5;
        tv.tv_usec = 0;

        setsockopt(clientFd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

        // ----------------------------------------------------
        // Doc request TCP
        // Ket thuc khi gap newline hoac nhan du JSON hoan chinh
        // ----------------------------------------------------
        std::string duLieu;
        char buffer[1024];

        bool quaDai = false;
        bool hetThoiGian = false;
        bool loiDoc = false;

        while (true) {
            ssize_t n = recv(clientFd, buffer, sizeof(buffer), 0);

            if (n == 0) {
                // Client dong ket noi
                break;
            }

            if (n < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    hetThoiGian = true;
                } else if (errno != EINTR) {
                    loiDoc = true;
                }

                if (errno == EINTR) {
                    continue;
                }

                break;
            }

            duLieu.append(buffer, static_cast<size_t>(n));

            if (duLieu.size() > MAX_LENH) {
                quaDai = true;
                break;
            }

            // Neu request co newline thi dung tai newline
            if (duLieu.find('\n') != std::string::npos) {
                break;
            }

            // Neu JSON da hoan chinh thi khong can cho newline
            if (json::accept(duLieu)) {
                break;
            }
        }

        if (quaDai) {
            ghiNhatKy(nguon + " | TU CHOI | Lenh qua dai");

            const std::string loi = "{\"status\":\"error\"," "\"message\":\"Message too large\"}\n";

            send(clientFd, loi.c_str(), loi.size(), 0);
            close(clientFd);
            continue;
        }

        if (duLieu.empty()) {
            if (hetThoiGian) {
                ghiNhatKy(nguon + " | LOI | Timeout khi doc request");
            } else if (loiDoc) {
                ghiNhatKy(nguon + " | LOI | Loi doc request");
            }

            close(clientFd);
            continue;
        }

        // Lay dong JSON dau tien
        size_t pos = duLieu.find('\n');

        std::string jsonStr = (pos == std::string::npos) ? duLieu : duLieu.substr(0, pos);

        json phanHoi;
        phanHoi["NodeId"] = docCauHinh("NodeId", "server-a");

        try {
            json outer = json::parse(jsonStr);

            // DEBUG: in cac key cua request
            if (outer.is_object()) {
                std::cout
                    << "[DEBUG TCP] Request tu "
                    << nguon << " | Keys:";

                for (auto it = outer.begin();
                     it != outer.end();
                     ++it) {
                    std::cout << " " << it.key();
                }

                std::cout << std::endl;
            } else {
                std::cout
                    << "[DEBUG TCP] Request khong phai JSON object tu "
                    << nguon << std::endl;
            }

            json j;

            bool xacThucThanhCong = false;
            std::string loaiGiaoThuc;
            std::string lyDoTuChoi = "Missing Payload or Signature";

            // =================================================
            // NHANH 1: REQUEST HMAC
            // =================================================
            if (
                outer.is_object()
                && outer.contains("Payload")
                && outer["Payload"].is_string()
                && outer.contains("Signature")
                && outer["Signature"].is_string()
            ) {
                loaiGiaoThuc = "HMAC";

                const std::string payloadStr = outer["Payload"].get<std::string>();

                const std::string clientSignature = outer["Signature"].get<std::string>();

                std::string serverSignature;

                bool tinhDuocHmac = tinhHmacSha256(payloadStr, SECRET_TOKEN, serverSignature);

                bool chuKyHopLe = tinhDuocHmac && chuKyBangNhauHangDinh(clientSignature, serverSignature);

                if (!tinhDuocHmac) {
                    lyDoTuChoi = "HMAC calculation failed";
                } else if (!chuKyHopLe) {
                    lyDoTuChoi = "Invalid HMAC signature";
                }

                if (chuKyHopLe) {
                    try {
                        j = json::parse(payloadStr);

                        // Timestamp phai nam trong Payload
                        if (j.is_object() && j.contains("Timestamp") && j["Timestamp"].is_number_integer()) {
                            const long long clientTime = j["Timestamp"].get<long long>();

                            const long long serverTime = static_cast<long long>(std::time(nullptr));

                            if (clientTime > 0 && std::llabs(serverTime - clientTime) <= 5) {
                                xacThucThanhCong = true;
                            } else {
                                lyDoTuChoi = "Packet expired";
                            }
                        } else {
                            lyDoTuChoi = "Missing or invalid Timestamp";
                        }
                    } catch (const std::exception&) {
                        lyDoTuChoi = "Malformed Payload JSON";
                    }
                }
            }

            // =================================================
            // NHANH 2: REQUEST TRUC TIEP TU WEB ADMIN
            // [MOI] Chi nhan khi RequireHmac khong bat
            // =================================================
            else if (
                !CHI_HMAC
                && outer.is_object()
                && outer.contains("AuthToken")
                && outer["AuthToken"].is_string()
                && outer.contains("CommandType")
                && outer["CommandType"].is_string()
            ) {
                loaiGiaoThuc = "AuthToken";

                const std::string clientToken = outer["AuthToken"].get<std::string>();

                // So sanh token theo thoi gian hang dinh
                bool tokenHopLe = chuKyBangNhauHangDinh(clientToken, SECRET_TOKEN);

                if (tokenHopLe) {
                    // Request truc tiep:
                    // JSON goc chinh la payload xu ly lenh
                    j = outer;
                    xacThucThanhCong = true;
                } else {
                    lyDoTuChoi = "Invalid AuthToken";
                }
            }

            // =================================================
            // NHANH 3: KHONG DUNG DINH DANG HO TRO
            // =================================================
            else {
                lyDoTuChoi = CHI_HMAC ? "HMAC signature required" : "Unsupported request format";
            }

            // =================================================
            // XU LY LENH CHUNG SAU KHI XAC THUC
            // =================================================
            if (xacThucThanhCong) {
                std::string cmd = j.value("CommandType", "");

                std::cout
                    << "[DEBUG TCP] Xac thuc OK"
                    << " | Protocol: " << loaiGiaoThuc
                    << " | CommandType: " << cmd
                    << std::endl;

                // -------------------------------------------------
                // GET METRICS
                // -------------------------------------------------
                if (cmd == "GetMetrics") {
                    SoLieu sl = laySoLieu();

                    phanHoi["status"] = "ok";
                    phanHoi["CPUUsagePercent"] = sl.cpuPhanTram;
                    phanHoi["TotalRAM"] = sl.tongRamMb;
                    phanHoi["UsedRAM"] = sl.daDungRamMb;
                    phanHoi["ProcessCount"] = sl.soTienTrinh;

                    std::vector<TienTrinhInfo> dsTT = layTienTrinh(30);

                    json mang = json::array();

                    for (const TienTrinhInfo& t : dsTT) {
                        json o;

                        o["name"] = t.ten;
                        o["pid"] = t.pid;
                        o["user"] = t.user;
                        o["ramMB"] = t.ramMb;

                        mang.push_back(o);
                    }

                    phanHoi["Processes"] = mang;

                    ghiNhatKy(nguon + " | OK | GetMetrics");
                }

                // -------------------------------------------------
                // UPDATE OS POLICY
                // [MOI] Them MaxCpuPercent va ReclaimRam
                // -------------------------------------------------
                else if (cmd == "UpdateOsPolicy") {
                    std::string target = j.value("TargetProcess", "");

                    std::string tenUser = j.value("TargetUser", "");

                    int maxRam = j.value("MaxRamMB", 0);

                    int maxCpu = j.value("MaxCpuPercent", 0);

                    bool thuHoi = j.value("ReclaimRam", false);

                    long uid = -1;

                    // Phai gioi han it nhat mot thu (RAM hoac CPU)
                    bool hopLe =
                        !target.empty()
                        && maxRam >= 0
                        && maxRam <= 1048576
                        && maxCpu >= 0
                        && maxCpu <= 6400
                        && (maxRam > 0 || maxCpu > 0);

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
                                    l.maxKb = static_cast<long>(maxRam) * 1024;
                                    l.maxCpu = maxCpu;
                                    l.thuHoiRam = thuHoi;

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
                                    moi.maxKb = static_cast<long>(maxRam) * 1024;
                                    moi.maxCpu = maxCpu;
                                    moi.thuHoiRam = thuHoi;

                                    danhSachLuat.push_back(moi);
                                }
                            }
                        }

                        if (thanhCong) {
                            std::string ten = tenUser.empty() ? std::string("moi user") : tenUser;

                            std::cout
                                << "[HOP LE] UpdateOsPolicy"
                                << " | user: " << ten
                                << " | process: " << target
                                << " | RAM " << maxRam << "MB"
                                << " | CPU " << maxCpu << "%"
                                << " | ThuHoi "
                                << (thuHoi ? "co" : "khong")
                                << std::endl;

                            ghiNhatKy(
                                nguon + " | OK | Luat "
                                + ten + "/" + target
                                + " | RAM " + std::to_string(maxRam) + "MB"
                                + " | CPU " + std::to_string(maxCpu) + "%"
                                + " | ThuHoi " + (thuHoi ? "co" : "khong")
                            );

                            phanHoi["status"] = "ok";
                            phanHoi["message"] = "Policy applied";
                        } else {
                            std::cout
                                << "[TU CHOI] Qua nhieu luat"
                                << std::endl;

                            ghiNhatKy(nguon + " | TU CHOI | Qua nhieu luat");

                            phanHoi["status"] = "error";
                            phanHoi["message"] = "Too many rules";
                        }
                    } else {
                        std::cout
                            << "[TU CHOI] Luat khong hop le "
                            << "hoac user khong ton tai"
                            << std::endl;

                        ghiNhatKy(nguon + " | TU CHOI | Luat khong hop le " "hoac user khong ton tai: " + tenUser);

                        phanHoi["status"] = "error";
                        phanHoi["message"] = "Invalid policy or unknown user";
                    }
                }

                // -------------------------------------------------
                // GET OS POLICIES
                // [MOI] Tra them MaxCpuPercent va ReclaimRam
                // -------------------------------------------------
                else if (cmd == "GetOsPolicies") {
                    json arr = json::array();

                    {
                        std::lock_guard<std::mutex> lg(khoa);

                        for (const LuatRieng& l : danhSachLuat) {
                            json o;

                            o["TargetUser"] = (l.uid == -1) ? std::string("") : tenTuUid(l.uid);

                            o["TargetProcess"] = l.tenTienTrinh;

                            o["MaxRamMB"] = l.maxKb / 1024;

                            o["MaxCpuPercent"] = l.maxCpu;
                            o["ReclaimRam"] = l.thuHoiRam;

                            arr.push_back(o);
                        }
                    }

                    phanHoi["status"] = "ok";
                    phanHoi["Policies"] = arr;

                    ghiNhatKy(nguon + " | OK | GetOsPolicies");
                }

                // -------------------------------------------------
                // REMOVE OS POLICY
                // -------------------------------------------------
                else if (cmd == "RemoveOsPolicy") {
                    std::string target = j.value("TargetProcess", "");

                    std::string tenUser = j.value("TargetUser", "");

                    long uid = -1;

                    bool hopLe = !target.empty();

                    if (hopLe && !tenUser.empty()) {
                        struct passwd* p = getpwnam(tenUser.c_str());

                        if (p == nullptr) {
                            hopLe = false;
                        } else {
                            uid = p->pw_uid;
                        }
                    }

                    if (!hopLe) {
                        phanHoi["status"] = "error";
                        phanHoi["message"] = "Invalid policy or unknown user";
                    } else {
                        size_t xoa = 0;

                        {
                            std::lock_guard<std::mutex> lg(khoa);

                            for (auto it = danhSachLuat.begin(); it != danhSachLuat.end();) {
                                if (it->uid == uid && it->tenTienTrinh == target) {
                                    it = danhSachLuat.erase(it);
                                    xoa++;
                                } else {
                                    ++it;
                                }
                            }
                        }

                        ghiNhatKy(
                            nguon + " | OK | Go luat "
                            + (tenUser.empty()
                                ? std::string("moi user")
                                : tenUser)
                            + "/" + target
                        );

                        phanHoi["status"] = xoa ? "ok" : "error";

                        phanHoi["message"] = xoa ? "Policy removed" : "Policy not found";
                    }
                }

                // -------------------------------------------------
                // SUBMIT JOB
                // -------------------------------------------------
                else if (cmd == "SubmitJob") {
                    phanHoi.update(xuLySubmitJob(j, nguon));
                }

                // -------------------------------------------------
                // GET JOB STATUS
                // -------------------------------------------------
                else if (cmd == "GetJobStatus") {
                    phanHoi.update(xuLyGetJobStatus(j));
                }

                // -------------------------------------------------
                // CANCEL JOB
                // -------------------------------------------------
                else if (cmd == "CancelJob") {
                    phanHoi.update(xuLyCancelJob(j, nguon));
                }

                // -------------------------------------------------
                // GET JOB OUTPUT
                // -------------------------------------------------
                else if (cmd == "GetJobOutput") {
                    std::string jobId = j.value("JobId", "");
                    std::string filename = j.value("Filename", "result.json");
                    
                    // Bảo mật: Chặn Path Traversal
                    if (jobId.empty() || filename.empty() || 
                        filename.find("..") != std::string::npos || 
                        filename.find("/") != std::string::npos) {
                        phanHoi["status"] = "error";
                        phanHoi["message"] = "Invalid path";
                    } else {
                        std::string filePath = "/tmp/qos-jobs/" + jobId + "/" + filename;
                        std::ifstream f(filePath, std::ios::binary);
                        if (!f.is_open()) {
                            phanHoi["status"] = "error";
                            phanHoi["message"] = "File not found";
                        } else {
                            std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
                            
                            phanHoi["status"] = "ok";
                            phanHoi["Filename"] = filename;
                            phanHoi["Size"] = content.size();
                            phanHoi["Content"] = base64Encode(content);
                            
                            ghiNhatKy(nguon + " | OK | GetJobOutput " + jobId + "/" + filename + " (" + std::to_string(content.size()) + " bytes)");
                        }
                    }
                }


                // -------------------------------------------------
                // UNKNOWN COMMAND
                // -------------------------------------------------
                else {
                    std::cout
                        << "[TU CHOI] Lenh khong ho tro: "
                        << cmd << std::endl;

                    ghiNhatKy(nguon + " | TU CHOI | Lenh khong ho tro: " + cmd);

                    phanHoi["status"] = "error";
                    phanHoi["message"] = "Unknown command";
                }
            }

            // =================================================
            // REQUEST KHONG QUA XAC THUC
            // =================================================
            else {
                std::cout
                    << "[TU CHOI] Xac thuc khong hop le: "
                    << lyDoTuChoi
                    << " | Protocol: " << loaiGiaoThuc
                    << " | IP: " << nguon
                    << std::endl;

                ghiNhatKy(nguon + " | TU CHOI | " + lyDoTuChoi);

                phanHoi["status"] = "error";
                phanHoi["message"] = lyDoTuChoi;
            }
        }
        catch (const std::exception& e) {
            std::cout
                << "[LOI CU PHAP] "
                << e.what()
                << std::endl;

            ghiNhatKy(nguon + " | TU CHOI | Malformed JSON");

            phanHoi["status"] = "error";
            phanHoi["message"] = "Malformed JSON";
        }

        // ----------------------------------------------------
        // Gui response JSON
        // ----------------------------------------------------
        std::string guiDi = phanHoi.dump() + "\n";

        size_t daGui = 0;

        while (daGui < guiDi.size()) {
            ssize_t n = send(clientFd, guiDi.data() + daGui, guiDi.size() - daGui, 0);

            if (n < 0) {
                if (errno == EINTR) {
                    continue;
                }
                break;
            }

            if (n == 0) {
                break;
            }

            daGui += static_cast<size_t>(n);
        }

        close(clientFd);
    }

    close(serverfd);
}
