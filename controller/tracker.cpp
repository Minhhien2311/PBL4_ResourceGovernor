#include "tracker.h"
#include "jobstore.h"
#include "logger.h"
#include "net_client.h"
#include "registry.h"
#include "scheduler.h"
#include <nlohmann/json.hpp>
#include <unistd.h>

using json = nlohmann::json;

static const int NGUONG_DUNG_CHUYEN_GIAY = 20;   // job bi tam dung qua lau thi chuyen sang server khac
static const int NGUONG_MAT_KET_NOI_GIAY = 45;   // mat ket noi qua lau thi giao lai
static const int MAX_LAN_THU = 3;

static bool hoiAgent(const ServerInfo& s, const std::string& lenh, const std::string& agentJobId,
                     json& kq, std::string& loi) {
    json yc;
    yc["CommandType"] = lenh;
    yc["JobId"] = agentJobId;
    yc["AuthToken"] = s.token;
    std::string ph;
    if (!guiLenh(s.ip, s.port, yc.dump(), ph, loi, 2, s.token))
        return false;
    try {
        kq = json::parse(ph);
    } catch (...) {
        loi = "phan hoi hong";
        return false;
    }
    return true;
}

void chayTracker(int chuKyGiay) {
    while (true) {
        std::vector<JobCtl> jobs = layTatCa();
        std::vector<ServerInfo> ds = layBanSao();
        time_t now = time(nullptr);

        for (const JobCtl& j : jobs) {
            if (j.state == "PENDING") { giaoJob(j.id); continue; }   // hang doi: thu giao lai moi chu ky
            if (j.state != "SUBMITTING" && j.state != "UNKNOWN" &&
                j.state != "RUNNING" && j.state != "SUSPENDED" &&
                j.state != "CANCELLING") continue;

            int idx = timServer(j.server);
            if (idx < 0) continue;
            const ServerInfo& s = ds[idx];

            json r;
            std::string loi;
            if (!hoiAgent(s, "GetJobStatus", j.agentJobId, r, loi)) {
                time_t tu = j.matKetNoiTu ? j.matKetNoiTu : now;
                suaJob(j.id, [&](JobCtl& x) { if (!x.matKetNoiTu) x.matKetNoiTu = now; });
                if (now - tu >= NGUONG_MAT_KET_NOI_GIAY && j.state != "UNKNOWN" && j.state != "CANCELLING") {
                    // Mat lien lac khong chung minh tien trinh da dung. Giu nguyen
                    // node va Agent JobId de doi soat, khong chay ban sao thu hai.
                    suaJob(j.id, [&](JobCtl& x) {
                        x.state = "UNKNOWN";
                        x.reason = "mat ket noi voi " + x.server + "; chua xac dinh job co con chay hay khong";
                    });
                    ghiNhatKy("JOB CAN DOI SOAT | " + j.id + " | " + j.server + " mat ket noi");
                }
                continue;
            }

            suaJob(j.id, [&](JobCtl& x) { x.matKetNoiTu = 0; });

            if (r.value("status", "") != "ok") {
                if (r.value("message", "") == "Job not found") {
                    // Sau khi Agent restart, bang theo doi co the mat trong khi
                    // process con van song. Khong tu dong submit lai trong truong hop nay.
                    if (j.state != "UNKNOWN") {
                        suaJob(j.id, [&](JobCtl& x) {
                            x.state = "UNKNOWN";
                            x.reason = "Agent khong con ban ghi job; can kiem tra tien trinh tren node";
                        });
                        ghiNhatKy("JOB CAN DOI SOAT | " + j.id + " | Agent khong con ban ghi job");
                    }
                }
                continue;
            }

            std::string st = r.value("State", "");
            if (st == "DONE" || st == "FAILED" || st == "CANCELLED") {
                int ma = r.value("ExitCode", 0);
                bool choChuyen = j.state == "CANCELLING" && j.reason.rfind("handoff:", 0) == 0 && st == "CANCELLED";
                suaJob(j.id, [&](JobCtl& x) {
                    if (choChuyen) {
                        x.state = "PENDING";
                        x.reason = "job cu da dung; cho giao lai";
                        x.server.clear();
                        x.agentJobId.clear();
                        x.dungTu = 0;
                        x.matKetNoiTu = 0;
                    } else {
                        x.state = st;
                    }
                });
                if (st != j.state) {
                    ghiNhatKy("JOB KET THUC | " + j.id + " | " + j.server + " | " + st + " | exit " + std::to_string(ma));
                }
                if (choChuyen) giaoJob(j.id);
            } else if (st == "SUSPENDED") {
                if (j.state == "CANCELLING") continue;
                time_t tu = j.dungTu ? j.dungTu : now;
                suaJob(j.id, [&](JobCtl& x) {
                    if (x.state == "CANCELLED" || x.state == "DONE" || x.state == "FAILED") return;
                    x.state = "SUSPENDED";
                    if (!x.dungTu) x.dungTu = now;
                });
                if (now - tu >= NGUONG_DUNG_CHUYEN_GIAY && j.lanThu < MAX_LAN_THU) {
                    KetQuaChon kq = chonServer(j.memoryMb, j.daThu);
                    if (kq.idx >= 0) {
                        json huy;
                        std::string l2;
                        if (!hoiAgent(s, "CancelJob", j.agentJobId, huy, l2)) {
                            ghiNhatKy("CHUYEN JOB THAT BAI | " + j.id + " | CancelJob loi: " + l2);
                            continue;   // giu SUSPENDED, thu lai chu ky sau
                        }
                        std::string trangThaiHuy = huy.value("status", "");
                        if (trangThaiHuy != "ok") {
                            std::string msgHuy = huy.value("message", "");
                            if (msgHuy == "Job not found") {
                                // Agent khong con track job (co the da restart).
                                // Job cu chac chan khong con chay → an toan move.
                                ghiNhatKy("CHUYEN JOB | " + j.id + " | agent mat job, cho phep move");
                            } else {
                                ghiNhatKy("CHUYEN JOB THAT BAI | " + j.id + " | CancelJob rejected: " + msgHuy);
                                continue;
                            }
                        }
                        std::string serverCu = j.server;
                        suaJob(j.id, [&](JobCtl& x) {
                            x.reason = "handoff: cho Agent xac nhan job dung truoc khi chuyen tu " + serverCu;
                            x.state = "CANCELLING";
                        });
                        ghiNhatKy("CHO HUY DE CHUYEN JOB | " + j.id + " | tu " + serverCu + " | " + kq.lyDo);
                    }
                }
            } else if (st == "RUNNING") {
                suaJob(j.id, [&](JobCtl& x) {
                    if (x.state == "CANCELLED" || x.state == "DONE" || x.state == "FAILED" || x.state == "CANCELLING") return;
                    x.state = "RUNNING";
                    x.dungTu = 0;
                });
            }
        }
        sleep(chuKyGiay);
    }
}
