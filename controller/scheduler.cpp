#include "scheduler.h"
#include "jobstore.h"
#include "logger.h"
#include "net_client.h"
#include "registry.h"
#include <algorithm>
#include <cstdio>
#include <ctime>
#include <mutex>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

static const long DU_RAM_MB = 150;        // luon chua lai chut RAM
static const double NGUONG_CPU = 85.0;    // CPU tren muc nay coi la qua tai
static const int GIU_CHO_GIAY = 30;       // job moi giao chua kip hien trong so lieu, tinh nhu da chiem RAM
static std::mutex mGiao;

static std::string soNguyen(double v) {
    char b[32];
    std::snprintf(b, sizeof(b), "%.0f", v);
    return b;
}

KetQuaChon chonServer(long memoryMb, const std::vector<std::string>& loaiTru) {
    std::vector<ServerInfo> ds = layBanSao();
    std::vector<JobCtl> jobs = layTatCa();
    time_t now = time(nullptr);

    KetQuaChon kq;
    double diemTot = 1e9;
    long ramChon = 0;
    double cpuChon = 0;
    std::string loai;   // moi muc bat dau bang "; "

    for (size_t i = 0; i < ds.size(); i++) {
        const ServerInfo& s = ds[i];
        std::string st = trangThai(s);
        if (st != "ONLINE") { loai += "; " + s.ten + " " + st; continue; }
        if (std::find(loaiTru.begin(), loaiTru.end(), s.ten) != loaiTru.end()) {
            loai += "; " + s.ten + " da thu";
            continue;
        }

        long giuCho = 0;
        int dangChay = 0;
        for (const JobCtl& j : jobs) {
            if (j.server != s.ten) continue;
            if (j.state == "RUNNING" || j.state == "SUSPENDED") {
                dangChay++;
                if (now - j.giaoLuc < GIU_CHO_GIAY) giuCho += j.memoryMb;
            }
        }
        long ramTrong = (s.tongRamMb - s.daDungRamMb) - giuCho;
        if (ramTrong < memoryMb + DU_RAM_MB) {
            loai += "; " + s.ten + " thieu RAM (trong " + std::to_string(ramTrong) + " MB, can " + std::to_string(memoryMb) + " MB)";
            continue;
        }
        if (s.cpu > NGUONG_CPU) {
            loai += "; " + s.ten + " qua tai CPU " + soNguyen(s.cpu) + "%";
            continue;
        }
        
        double ramPercent = s.tongRamMb > 0
            ? (double)(s.daDungRamMb + giuCho) / (double)s.tongRamMb * 100.0
            : 0.0;
        double diem = 0.5 * s.cpu + 0.5 * ramPercent + 10.0 * dangChay;
        if (diem < diemTot) {
            diemTot = diem;
            kq.idx = (int)i;
            ramChon = ramTrong;
            cpuChon = s.cpu;
        }
    }

    if (kq.idx >= 0) {
        kq.lyDo = "chon " + ds[kq.idx].ten + " (CPU " + soNguyen(cpuChon) + "%, RAM trong " + std::to_string(ramChon) + " MB)";
        if (!loai.empty()) kq.lyDo += "; loai:" + loai.substr(1);
    } else {
        kq.lyDo = loai.empty() ? std::string("khong co server nao") : "khong co server phu hop:" + loai.substr(1);
    }
    return kq;
}

bool giaoJob(const std::string& id) {
    std::lock_guard<std::mutex> lg(mGiao);   // chi mot luong giao job tai mot thoi diem, tranh giao trung
    JobCtl j;
    if (!layJob(id, j) || j.state != "PENDING") return false;

    std::vector<ServerInfo> ds = layBanSao();
    std::vector<std::string> loaiTru = j.daThu;
    std::string lyDoCuoi;

    for (size_t lan = 0; lan <= ds.size(); lan++) {
        KetQuaChon kq = chonServer(j.memoryMb, loaiTru);
        lyDoCuoi = kq.lyDo;
        if (kq.idx < 0) break;
        const ServerInfo& s = ds[kq.idx];

        const int soLan = j.lanThu + 1;
        std::string agentId = j.id + "-" + std::to_string(soLan);
        json yc;
        yc["CommandType"] = "SubmitJob";
        yc["JobId"] = agentId;
        yc["JobType"] = j.loai;
        yc["TargetUser"] = j.user;
        yc["Params"] = j.params;
        yc["AuthToken"] = s.token;

        // Ghi lai node va ID truoc khi gui. Neu mat ACK, tracker se doi soat
        // dung job nay thay vi tu dong tao mot job moi tren node khac.
        suaJob(id, [&](JobCtl& x) {
            x.state = "SUBMITTING";
            x.server = s.ten;
            x.agentJobId = agentId;
            x.lanThu = soLan;
            x.giaoLuc = time(nullptr);
            x.reason = "dang gui len Agent";
            x.daThu.push_back(s.ten);
        });

        std::string ph, loi, chiTiet;
        bool thanhCong = false;
        bool biTuChoiRoRang = false;
        if (!guiLenh(s.ip, s.port, yc.dump(), ph, loi, 2, s.token)) {
            chiTiet = "ket qua SubmitJob chua xac dinh: " + loi;
            // connect() that bai nghia la payload chua duoc gui; co the an toan
            // thu node khac. Loi send/recv van duoc xem la ket qua chua ro.
            biTuChoiRoRang = loi.rfind("connect:", 0) == 0 ||
                             loi.rfind("socket:", 0) == 0 ||
                             loi.rfind("IP khong hop le:", 0) == 0;
        } else {
            try {
                json r = json::parse(ph);
                if (r.value("status", "") == "ok") thanhCong = true;
                else {
                    biTuChoiRoRang = true;
                    chiTiet = r.value("message", "agent tu choi");
                }
            } catch (...) {
                chiTiet = "phan hoi SubmitJob hong, ket qua chua xac dinh";
            }
        }

        if (thanhCong) {
            std::string lyDo = kq.lyDo;
            std::string tenServer = s.ten;
            suaJob(id, [&](JobCtl& x) {
                x.server = tenServer;
                x.agentJobId = agentId;
                x.state = "RUNNING";
                x.reason = lyDo;
                x.giaoLuc = time(nullptr);
                x.dungTu = 0;
                x.matKetNoiTu = 0;
            });
            ghiNhatKy("GIAO JOB | " + id + " -> " + tenServer + " | " + lyDo);
            return true;
        }
        ghiNhatKy("GIAO JOB THAT BAI | " + id + " | " + s.ten + " | " + chiTiet);
        if (!biTuChoiRoRang) {
            suaJob(id, [&](JobCtl& x) {
                x.state = "UNKNOWN";
                x.reason = chiTiet + "; can doi soat tren " + s.ten;
            });
            return false;
        }

        // Agent da tra loi tu choi, nen chac chan chua chap nhan job.
        loaiTru.push_back(s.ten);
        lyDoCuoi = s.ten + ": " + chiTiet;
        suaJob(id, [&](JobCtl& x) {
            x.state = "PENDING";
            x.server.clear();
            x.agentJobId.clear();
            x.reason = lyDoCuoi;
        });
    }

    suaJob(id, [&](JobCtl& x) { x.reason = "cho: " + lyDoCuoi; });
    return false;
}
