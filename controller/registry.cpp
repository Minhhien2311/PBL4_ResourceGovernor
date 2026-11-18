
#include "registry.h"
#include <fstream>
#include <mutex>
#include <sstream>

static std::mutex m;
static std::vector<ServerInfo> danhSach;

static std::string cat(const std::string& s) {
    size_t a = s.find_first_not_of(" \t\r");
    if (a == std::string::npos) return "";
    size_t b = s.find_last_not_of(" \t\r");
    return s.substr(a, b - a + 1);
}

bool napDanhSach(const std::string& tenFile) {
    std::ifstream f(tenFile);
    if (!f.is_open()) return false;
    std::vector<ServerInfo> moi;
    std::string dong;
    while (std::getline(f, dong)) {
        dong = cat(dong);
        if (dong.empty() || dong[0] == '#') continue;
        std::vector<std::string> phan;
        std::stringstream ss(dong);
        std::string o;
        while (std::getline(ss, o, ',')) phan.push_back(cat(o));
        // Dinh dang moi: ten,ip,port_os,port_network,token
        // Van doc duoc dinh dang cu 4 cot: ten,ip,port_os,token (port_network mac dinh 9002)
        if (phan.size() != 4 && phan.size() != 5) continue;
        ServerInfo s;
        s.ten = phan[0];
        s.ip = phan[1];
        try {
            s.port = std::stoi(phan[2]);
            if (phan.size() == 5) s.port_network = std::stoi(phan[3]);
        } catch (...) { continue; }
        if (s.port < 1 || s.port > 65535 || s.port_network < 1 || s.port_network > 65535) continue;
        s.token = phan[phan.size() - 1];
        moi.push_back(s);
    }
    std::lock_guard<std::mutex> lg(m);
    danhSach = moi;
    return !danhSach.empty();
}

std::vector<ServerInfo> layBanSao() {
    std::lock_guard<std::mutex> lg(m);
    return danhSach;
}

bool capNhatThanhCong(size_t idx, double cpu, long tong, long dung, int soTT) {
    std::lock_guard<std::mutex> lg(m);
    if (idx >= danhSach.size()) return false;
    ServerInfo& s = danhSach[idx];
    bool phucHoi = !s.loiCuoi.empty();
    s.daTungKetNoi = true;
    s.lanCuoi = time(nullptr);
    s.cpu = cpu;
    s.tongRamMb = tong;
    s.daDungRamMb = dung;
    s.soTienTrinh = soTT;
    s.loiCuoi.clear();
    return phucHoi;
}

bool capNhatThatBai(size_t idx, const std::string& loi) {
    std::lock_guard<std::mutex> lg(m);
    if (idx >= danhSach.size()) return false;
    ServerInfo& s = danhSach[idx];
    bool loiMoi = s.loiCuoi.empty();
    s.loiCuoi = loi;
    return loiMoi;
}

void capNhatTienTrinh(size_t idx, const std::vector<TienTrinh>& ds) {
    std::lock_guard<std::mutex> lg(m);
    if (idx < danhSach.size()) danhSach[idx].dsTienTrinh = ds;
}

std::string trangThai(const ServerInfo& s) {
    if (!s.daTungKetNoi) return "CHUA KET NOI";
    long tuoi = (long)(time(nullptr) - s.lanCuoi);
    if (tuoi < 15) return "ONLINE";
    if (tuoi < 30) return "DEGRADED";
    return "OFFLINE";
}

int timServer(const std::string& ten) {
    std::lock_guard<std::mutex> lg(m);
    for (size_t i = 0; i < danhSach.size(); i++) {
        if (danhSach[i].ten == ten) return (int)i;
    }
    return -1;
}