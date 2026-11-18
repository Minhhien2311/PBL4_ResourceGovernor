// #ifndef REGISTRY_H
// #define REGISTRY_H
// #include <ctime>
// #include <string>
// #include <vector>

// struct ServerInfo {
//     std::string ten;
//     std::string ip;
//     int port = 9001;             // OS Agent (TV1)
//     int port_network = 9002;     // Network Proxy (TV2)
//     std::string token;
//     bool daTungKetNoi = false;
//     time_t lanCuoi = 0;          // lan cuoi nhan duoc so lieu hop le
//     double cpu = 0;
//     long tongRamMb = 0;
//     long daDungRamMb = 0;
//     int soTienTrinh = 0;
//     std::string loiCuoi;         // rong nghia la lan hoi gan nhat thanh cong
// };

// bool napDanhSach(const std::string& tenFile);
// std::vector<ServerInfo> layBanSao();
// bool capNhatThanhCong(size_t idx, double cpu, long tong, long dung, int soTT);   // true neu vua phuc hoi sau loi
// bool capNhatThatBai(size_t idx, const std::string& loi);                          // true neu day la loi moi
// std::string trangThai(const ServerInfo& s);
// int timServer(const std::string& ten);

// #endif

#ifndef REGISTRY_H
#define REGISTRY_H
#include <ctime>
#include <string>
#include <vector>

// Mot tien trinh tren server (lay tu phan hoi GetMetrics cua Agent)
struct TienTrinh {
    std::string ten;
    int pid = 0;
    std::string user;
    double ramMb = 0;
};

struct ServerInfo {
    std::string ten;
    std::string ip;
    int port = 9001;             // OS Agent (TV1)
    int port_network = 9002;     // Network Proxy (TV2)
    std::string token;
    bool daTungKetNoi = false;
    time_t lanCuoi = 0;          // lan cuoi nhan duoc so lieu hop le
    double cpu = 0;
    long tongRamMb = 0;
    long daDungRamMb = 0;
    int soTienTrinh = 0;
    std::string loiCuoi;         // rong nghia la lan hoi gan nhat thanh cong
    std::vector<TienTrinh> dsTienTrinh;   // top tien trinh theo RAM, cap nhat moi lan poll thanh cong
};

bool napDanhSach(const std::string& tenFile);
std::vector<ServerInfo> layBanSao();
bool capNhatThanhCong(size_t idx, double cpu, long tong, long dung, int soTT);   // true neu vua phuc hoi sau loi
bool capNhatThatBai(size_t idx, const std::string& loi);                          // true neu day la loi moi
void capNhatTienTrinh(size_t idx, const std::vector<TienTrinh>& ds);              // luu danh sach tien trinh moi nhat
std::string trangThai(const ServerInfo& s);
int timServer(const std::string& ten);

#endif