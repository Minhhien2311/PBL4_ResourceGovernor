// #include "monitor.h"
// #include "shared.h"
// #include <dirent.h>
// #include <cctype>
// #include <fstream>
// #include <sstream>
// #include <unistd.h>
// #include <unordered_set>
// #include <signal.h>
// #include <sys/types.h>
// #include <iostream>

// void chayMonitorLoop() {
//     std::unordered_set<int> danhsachDaSuspend;
//     while (true) {
//         std::string tenCanQuanLy;
//         long soKbMaxHienTai;
//         {
//             std::lock_guard<std::mutex> lg(khoa);
//             tenCanQuanLy = tenDangQuanLy;
//             soKbMaxHienTai = soKbMaxDungChung;
//         }

//         std::string dd = "/proc/meminfo";
//         std::ifstream file(dd);
//         if (!file.is_open()){
//             std::cout << "khong the mo file" << std::endl;
//         }
//         std::string dong;
//         long memTotal = 0;
//         long memAvailable = 0;
//         while(std::getline(file, dong)) {
//             if (dong.substr(0, 9) == "MemTotal:"){
//                     std::istringstream iss(dong); 
//                     std::string nhan;
//                     std::string donvi;
//                     iss >> nhan >> memTotal >> donvi ;
//             }
//             if (dong.substr(0, 13) == "MemAvailable:"){
//                     std::istringstream iss(dong); 
//                     std::string nhan; 
//                     std::string donvi;
//                     iss >> nhan >> memAvailable >> donvi ;
//             }
//         }
//         double phanTramTong = double(memAvailable) / double(memTotal) * 100 ;
//         std::cout << "RAM he thong con trong: " << phanTramTong << "%" << std::endl;

//         DIR* dir = opendir("/proc");
//         if (dir == nullptr){
//             std::cerr <<("Loi roi nhe, opendir khong duoc!");
//             continue ;
//         }
//         struct dirent* entry;
//         while ((entry = readdir(dir)) != nullptr){
//             bool laPid = true;
//             std::string ten = entry->d_name;
//             std::string duongDan = "/proc/" + ten + "/status";
//             for(int i = 0; i < ten.length(); i++){
//                 char c = ten[i];
//                 if(isdigit(c)){
//                 }
//                 else{
//                     laPid = false;
//                     break;
//                 }
//             }
//             if (laPid){
//                 std::ifstream file(duongDan);
//                 if (!file.is_open()){
//                     std::cout << "khong the mo file" << std::endl;
//                 }
//                 std::string dong;
//                 std::string tenTienTrinh;   
//                 while (std::getline(file, dong)) {
//                     if (dong.substr(0, 5) == "Name:"){
//                         std::istringstream iss(dong);
//                         std::string nhan;
//                         iss >> nhan >> tenTienTrinh;
//                     }
                     
//                     if (dong.substr(0, 6) == "VmRSS:" && tenTienTrinh == tenCanQuanLy) {
//                         std::istringstream iss(dong); 
//                         std::string nhan;
//                         long soluong; 
//                         std::string donvi;
//                         iss >> nhan >> soluong >> donvi ;
//                         int soPid = std::stoi(ten);   // stoi = "string to integer"
//                         if (soluong >= soKbMaxHienTai && danhsachDaSuspend.count(soPid) == 0 && phanTramTong <= 29){
//                             int temp = kill(soPid, SIGSTOP);
//                             if (temp == 0){
//                                 danhsachDaSuspend.insert(soPid);
//                                 std::cout << "Da tam dung thanh cong tien trinh co PID: " << soPid << std::endl;
//                             }
//                         }
//                         if (danhsachDaSuspend.count(soPid) == 1 && phanTramTong >= 32){
//                             int cont = kill(soPid, SIGCONT);
//                             if (cont == 0){
//                                 danhsachDaSuspend.erase(soPid);
//                                 std::cout << "Cho tiep tuc tien trinh co PID: " << soPid << std::endl;
//                             }
//                         }
//                         break;
//                     }
//                 }
                
//             }
//         }
//         sleep(3);
//     }
// }

/*
#include "monitor.h"
#include "shared.h"
#include <dirent.h>
#include <cctype>
#include <fstream>
#include <sstream>
#include <unistd.h>
#include <unordered_set>
#include <signal.h>
#include <sys/types.h>
#include <iostream>

void chayMonitorLoop() {
    std::unordered_set<int> danhsachDaSuspend;
    while (true) {
        std::string tenCanQuanLy;
        long soKbMaxHienTai;
        std::string hanhDongHienTai; // Biến nhận lệnh thủ công
        {
            std::lock_guard<std::mutex> lg(khoa);
            tenCanQuanLy = tenDangQuanLy;
            soKbMaxHienTai = soKbMaxDungChung;
            hanhDongHienTai = hanhDong;
        }

        std::string dd = "/proc/meminfo";
        std::ifstream file(dd);
        if (!file.is_open()){
            std::cout << "khong the mo file" << std::endl;
        }
        std::string dong;
        long memTotal = 0;
        long memAvailable = 0;
        while(std::getline(file, dong)) {
            if (dong.substr(0, 9) == "MemTotal:"){
                    std::istringstream iss(dong); 
                    std::string nhan;
                    std::string donvi;
                    iss >> nhan >> memTotal >> donvi ;
            }
            if (dong.substr(0, 13) == "MemAvailable:"){
                    std::istringstream iss(dong); 
                    std::string nhan; 
                    std::string donvi;
                    iss >> nhan >> memAvailable >> donvi ;
            }
        }
        double phanTramTong = double(memAvailable) / double(memTotal) * 100 ;
        std::cout << "RAM he thong con trong: " << phanTramTong << "%" << std::endl;

        DIR* dir = opendir("/proc");
        if (dir == nullptr){
            std::cerr <<("Loi roi nhe, opendir khong duoc!");
            continue ;
        }
        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr){
            bool laPid = true;
            std::string ten = entry->d_name;
            std::string duongDan = "/proc/" + ten + "/status";
            for(int i = 0; i < ten.length(); i++){
                char c = ten[i];
                if(isdigit(c)){
                }
                else{
                    laPid = false;
                    break;
                }
            }
            if (laPid){
                std::ifstream file(duongDan);
                if (!file.is_open()){
                    std::cout << "khong the mo file" << std::endl;
                }
                std::string dong;
                std::string tenTienTrinh;   
                while (std::getline(file, dong)) {
                    if (dong.substr(0, 5) == "Name:"){
                        std::istringstream iss(dong);
                        std::string nhan;
                        iss >> nhan >> tenTienTrinh;
                    }
                     
                    if (dong.substr(0, 6) == "VmRSS:" && tenTienTrinh == tenCanQuanLy) {
                        std::istringstream iss(dong); 
                        std::string nhan;
                        long soluong; 
                        std::string donvi;
                        iss >> nhan >> soluong >> donvi ;
                        int soPid = std::stoi(ten);   

                        // ==== CHỈ SỬA KHỐI NÀY ĐỂ KẾT HỢP LỆNH TỪ WEB ADMIN ====
                        if (soluong >= soKbMaxHienTai) {
                            if (hanhDongHienTai == "Kill") {
                                kill(soPid, SIGKILL);
                                std::cout << "Da kill tien trinh co PID: " << soPid << std::endl;
                            } 
                            else if (hanhDongHienTai == "Pause" && danhsachDaSuspend.count(soPid) == 0 ) {
                                int temp = kill(soPid, SIGSTOP);
                                if (temp == 0){
                                    danhsachDaSuspend.insert(soPid);
                                    std::cout << "Da tam dung thanh cong tien trinh co PID: " << soPid << std::endl;
                                }
                            }
                        }

                        // Tự động Resume khi RAM tổng hệ thống ổn định trở lại
                        if (hanhDongHienTai == "Pause" && danhsachDaSuspend.count(soPid) == 1 && soluong <= soKbMaxHienTai ) {
                            int cont = kill(soPid, SIGCONT);
                            if (cont == 0){
                                danhsachDaSuspend.erase(soPid);
                                std::cout << "Cho tiep tuc tien trinh co PID: " << soPid << std::endl;
                            }
                        }
                        // ==========================================================
                        break;
                    }
                }
                
            }
        }
        sleep(3);
    }
}
    */



/*
#include "monitor.h"
#include "shared.h"
#include <dirent.h>
#include <cctype>
#include <fstream>
#include <sstream>
#include <unistd.h>
#include <unordered_set>
#include <signal.h>
#include <sys/types.h>
#include <iostream>

void chayMonitorLoop() {
    std::unordered_set<int> danhsachDaSuspend;

    while (true) {
        // 1. Lấy policy hiện tại (thread-safe)
        std::string tenCanQuanLy;
        long soKbMaxHienTai;
        std::string hanhDongHienTai;
        {
            std::lock_guard<std::mutex> lg(khoa);
            tenCanQuanLy = tenDangQuanLy;
            soKbMaxHienTai = soKbMaxDungChung;
            hanhDongHienTai = hanhDong;
        }

        // 2. Đọc thông tin RAM hệ thống (chỉ để in log)
        {
            std::ifstream file("/proc/meminfo");
            std::string dong;
            long memTotal = 0, memAvailable = 0;
            while (std::getline(file, dong)) {
                if (dong.substr(0, 9) == "MemTotal:") {
                    std::istringstream iss(dong);
                    std::string nhan, donvi;
                    iss >> nhan >> memTotal >> donvi;
                }
                if (dong.substr(0, 13) == "MemAvailable:") {
                    std::istringstream iss(dong);
                    std::string nhan, donvi;
                    iss >> nhan >> memAvailable >> donvi;
                }
            }
            if (memTotal > 0) {
                double phanTramTong = double(memAvailable) / double(memTotal) * 100;
                std::cout << "RAM he thong con trong: " << phanTramTong << "%" << std::endl;
            }
        }

        // 3. Quét tất cả tiến trình trong /proc
        DIR* dir = opendir("/proc");
        if (dir == nullptr) {
            std::cerr << "Loi: khong mo duoc /proc" << std::endl;
            sleep(3);
            continue;
        }

        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr) {
            std::string ten = entry->d_name;
            if (ten.empty() || !isdigit(ten[0])) continue;

            // Kiểm tra toàn bộ tên là số (là PID hợp lệ)
            bool laPid = true;
            for (char c : ten) {
                if (!isdigit(c)) { laPid = false; break; }
            }
            if (!laPid) continue;

            std::string duongDan = "/proc/" + ten + "/status";
            std::ifstream file(duongDan);
            if (!file.is_open()) continue;

            std::string dong;
            std::string tenTienTrinh;
            long soluong = -1;

            while (std::getline(file, dong)) {
                if (dong.substr(0, 5) == "Name:") {
                    std::istringstream iss(dong);
                    std::string nhan;
                    iss >> nhan >> tenTienTrinh;
                }
                if (dong.substr(0, 6) == "VmRSS:") {
                    std::istringstream iss(dong);
                    std::string nhan, donvi;
                    iss >> nhan >> soluong >> donvi;
                    break;  // Đã đủ thông tin cần thiết
                }
            }

            // 4. Chỉ xử lý tiến trình khớp tên policy
            if (tenTienTrinh != tenCanQuanLy) continue;
            if (soluong < 0) continue;

            int soPid = std::stoi(ten);

            // 5. Logic chính: so sánh RAM tiến trình với ngưỡng
            bool dangPause = (danhsachDaSuspend.count(soPid) == 1);

            if (hanhDongHienTai == "Kill") {
                if (soluong > soKbMaxHienTai) {
                    kill(soPid, SIGKILL);
                    std::cout << "[Kill] Da kill PID " << soPid 
                              << " (RAM " << soluong / 1024 << "MB > "
                              << soKbMaxHienTai / 1024 << "MB)" << std::endl;
                    danhsachDaSuspend.erase(soPid);
                }
            }
            else if (hanhDongHienTai == "Pause") {
                if (soluong > soKbMaxHienTai) {
                    // Vượt ngưỡng → pause nếu chưa pause
                    if (!dangPause) {
                        if (kill(soPid, SIGSTOP) == 0) {
                            danhsachDaSuspend.insert(soPid);
                            std::cout << "[Pause] Da tam dung PID " << soPid
                                      << " (RAM " << soluong / 1024 << "MB > "
                                      << soKbMaxHienTai / 1024 << "MB)" << std::endl;
                        } else {
                            std::cerr << "Khong the SIGSTOP PID " << soPid << std::endl;
                        }
                    }
                }
                else {
                    // Không vượt ngưỡng → resume nếu đang pause
                    if (dangPause) {
                        if (kill(soPid, SIGCONT) == 0) {
                            danhsachDaSuspend.erase(soPid);
                            std::cout << "[Resume] Cho tiep tuc PID " << soPid
                                      << " (RAM " << soluong / 1024 << "MB <= "
                                      << soKbMaxHienTai / 1024 << "MB)" << std::endl;
                        } else {
                            std::cerr << "Khong the SIGCONT PID " << soPid << std::endl;
                        }
                    }
                }
            }
            else if (hanhDongHienTai == "Notify") {
                if (soluong > soKbMaxHienTai) {
                    std::cout << "[Notify] PID " << soPid
                              << " vuot nguong (RAM " << soluong / 1024
                              << "MB > " << soKbMaxHienTai / 1024 << "MB)" << std::endl;
                }
            }
            // PauseAutoResume và Penalty: tạm thời xử lý như Pause
            else {
                if (soluong > soKbMaxHienTai && !dangPause) {
                    if (kill(soPid, SIGSTOP) == 0) {
                        danhsachDaSuspend.insert(soPid);
                        std::cout << "[Pause] PID " << soPid << " tam dung" << std::endl;
                    }
                }
                else if (soluong <= soKbMaxHienTai && dangPause) {
                    if (kill(soPid, SIGCONT) == 0) {
                        danhsachDaSuspend.erase(soPid);
                        std::cout << "[Resume] PID " << soPid << " tiep tuc" << std::endl;
                    }
                }
            }
        }
        closedir(dir);

        // 6. Dọn các PID đã chết khỏi danh sách pause
        for (auto it = danhsachDaSuspend.begin(); it != danhsachDaSuspend.end(); ) {
            if (kill(*it, 0) != 0) {
                it = danhsachDaSuspend.erase(it);
            } else {
                ++it;
            }
        }

        sleep(3);
    }
}

*/
#include "monitor.h"
#include "shared.h"
#include "logger.h"
#include <cctype>
#include <cerrno>
#include <csignal>
#include <cstring>
#include <ctime>
#include <dirent.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <sys/types.h>
#include <unistd.h>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "job.h"
const int TTL_GIAY = 20;                 // thoi gian toi thieu bi dung truoc khi duoc xet tha
const double MOC_THA_PHAN_TRAM = 32;     // RAM he thong trong >= muc nay thi coi la thoang

static double docPhanTramRamTrong() {
    std::ifstream file("/proc/meminfo");
    if (!file.is_open()) return -1;
    std::string dong;
    long memTotal = 0, memAvailable = 0;
    while (std::getline(file, dong)) {
        if (dong.substr(0, 9) == "MemTotal:") {
            std::istringstream iss(dong);
            std::string nhan, donvi;
            iss >> nhan >> memTotal >> donvi;
        }
        if (dong.substr(0, 13) == "MemAvailable:") {
            std::istringstream iss(dong);
            std::string nhan, donvi;
            iss >> nhan >> memAvailable >> donvi;
        }
    }
    if (memTotal <= 0) return -1;
    return double(memAvailable) / double(memTotal) * 100;
}

void chayMonitorLoop() {
    std::unordered_map<int, time_t> daDungLuc;   // PID -> thoi diem bi dung
    std::unordered_set<int> daBaoLoi;            // PID da ghi loi kill that bai, tranh ghi lap moi 3 giay

    ghiNhatKy("Monitor khoi dong | TTL " + std::to_string(TTL_GIAY) + "s | moc tha "
              + std::to_string((int)MOC_THA_PHAN_TRAM) + "%");

    while (true) {
        std::vector<LuatRieng> luat;
        {
            std::lock_guard<std::mutex> lg(khoa);
            luat = danhSachLuat;
        }

        double phanTramTrong = docPhanTramRamTrong();
        std::cout << "RAM he thong con trong: " << phanTramTrong << "%" << std::endl;

        std::unordered_set<int> conSong;

        DIR* dir = opendir("/proc");
        if (dir == nullptr) {
            std::cerr << "Loi: opendir khong duoc!" << std::endl;
            sleep(3);
            continue;
        }

        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr) {
            std::string ten = entry->d_name;
            bool laPid = !ten.empty();
            for (char c : ten) {
                if (!isdigit(static_cast<unsigned char>(c))) { laPid = false; break; }
            }
            if (!laPid) continue;

            std::ifstream file("/proc/" + ten + "/status");
            if (!file.is_open()) continue;

            std::string dong, tenTienTrinh;
            long uidTienTrinh = -1;
            long vmrss = -1;
            while (std::getline(file, dong)) {
                if (dong.substr(0, 5) == "Name:") {
                    std::istringstream iss(dong);
                    std::string nhan;
                    iss >> nhan >> tenTienTrinh;
                }
                if (dong.substr(0, 4) == "Uid:") {
                    std::istringstream iss(dong);
                    std::string nhan;
                    iss >> nhan >> uidTienTrinh;
                }
                if (dong.substr(0, 6) == "VmRSS:") {
                    std::istringstream iss(dong);
                    std::string nhan, donvi;
                    iss >> nhan >> vmrss >> donvi;
                }
            }

            int soPid = std::stoi(ten);
            conSong.insert(soPid);

            // Bao ve: khong dong vao init, chinh agent, tien trinh cua root, kernel thread
            if (soPid == 1 || soPid == getpid() || uidTienTrinh == 0 || vmrss < 0) continue;

            // Tim gioi han ap dung: luat rieng cua user uu tien hon luat chung (uid = -1)
            long gioiHanRieng = -1, gioiHanChung = -1;
            for (const LuatRieng& l : luat) {
                if (l.tenTienTrinh != tenTienTrinh) continue;
                if (l.uid == uidTienTrinh) gioiHanRieng = l.maxKb;
                else if (l.uid == -1) gioiHanChung = l.maxKb;
            }
            long gioiHan = (gioiHanRieng != -1) ? gioiHanRieng : gioiHanChung;

            bool daDung = daDungLuc.count(soPid) > 0;
            const std::string moTa = "PID " + std::to_string(soPid) + " | " + tenTienTrinh
                                     + " | uid " + std::to_string(uidTienTrinh);

            if (gioiHan != -1 && vmrss >= gioiHan && !daDung) {
                if (kill(soPid, SIGSTOP) == 0) {
                    daDungLuc[soPid] = time(nullptr);
                    daBaoLoi.erase(soPid);
                    std::cout << "Da tam dung " << moTa << " | " << vmrss / 1024 << " MB" << std::endl;
                    ghiNhatKy("DUNG | " + moTa + " | " + std::to_string(vmrss / 1024)
                              + " MB | gioi han " + std::to_string(gioiHan / 1024) + " MB");
                } else if (daBaoLoi.count(soPid) == 0) {
                    daBaoLoi.insert(soPid);
                    ghiNhatKy("LOI | Khong dung duoc " + moTa + " | " + std::strerror(errno));
                }
            } else if (daDung) {
                bool hetVuot = (gioiHan == -1 || vmrss < gioiHan);
                bool hetHan = (time(nullptr) - daDungLuc.at(soPid) >= TTL_GIAY) && phanTramTrong >= MOC_THA_PHAN_TRAM;
                if (hetVuot || hetHan) {
                    if (kill(soPid, SIGCONT) == 0) {
                        std::string lyDo;
                        if (gioiHan == -1) lyDo = "luat bi go";
                        else if (vmrss < gioiHan) lyDo = "han muc nang len hoac RAM giam";
                        else lyDo = "het han TTL";
                        daDungLuc.erase(soPid);
                        daBaoLoi.erase(soPid);
                        std::cout << "Cho chay tiep " << moTa << std::endl;
                        ghiNhatKy("THA | " + moTa + " | ly do: " + lyDo);
                    } else if (daBaoLoi.count(soPid) == 0) {
                        daBaoLoi.insert(soPid);
                        ghiNhatKy("LOI | Khong tha duoc " + moTa + " | " + std::strerror(errno));
                    }
                }
            }
        }
        closedir(dir);

        // Don dep PID khong con ton tai
        for (auto it = daDungLuc.begin(); it != daDungLuc.end(); ) {
            if (conSong.count(it->first) == 0) {
                ghiNhatKy("DON DEP | PID " + std::to_string(it->first) + " da thoat khi dang bi dung");
                it = daDungLuc.erase(it);
            } else {
                ++it;
            }
        }
        for (auto it = daBaoLoi.begin(); it != daBaoLoi.end(); ) {
            if (conSong.count(*it) == 0) it = daBaoLoi.erase(it);
            else ++it;
        }
        
        thuHoiJob();
        sleep(3);
    }
}