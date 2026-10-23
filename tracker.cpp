#include "tracker.h"
#include "jobstore.h"
#include "logger.h"
#include "net_client.h"
#include "registry.h"
#include "scheduler.h"
#include <nlohmann/json.hpp>
#include <unistd.h>

using json = nlohmann::json;

static const int NGUONG_DUNG_CHUYEN_GIAY = 30;   // job bi tam dung qua lau thi chuyen sang server khac
static const int NGUONG_MAT_KET_NOI_GIAY = 45;   // mat ket noi qua lau thi giao lai
static const int MAX_LAN_THU = 3;

static bool hoiAgent(const ServerInfo& s, const std::string& lenh, const std::string& agentJobId,
                     json& kq, std::string& loi) {
    json yc;
    yc["CommandType"] = lenh;
    yc["JobId"] = agentJobId;
    yc["AuthToken"] = s.token;
    std::string ph;
    if (!guiLenh(s.ip, s.port, yc.dump(), ph, loi, 2)) return false;
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
            if (j.state != "RUNNING" && j.state != "SUSPENDED") continue;

            int idx = timServer(j.server);
            if (idx < 0) continue;
            const ServerInfo& s = ds[idx];

            json r;
            std::string loi;
            if (!hoiAgent(s, "GetJobStatus", j.agentJobId, r, loi)) {
                time_t tu = j.matKetNoiTu ? j.matKetNoiTu : now;
                suaJob(j.id, [&](JobCtl& x) { if (!x.matKetNoiTu) x.matKetNoiTu = now; });
                if (now - tu >= NGUONG_MAT_KET_NOI_GIAY) {
                    if (j.lanThu < MAX_LAN_THU) {
                        ghiNhatKy("GIAO LAI | " + j.id + " | " + j.server + " mat ket noi");
                        suaJob(j.id, [&](JobCtl& x) {
                            x.reason = "giao lai: " + x.server + " mat ket noi";
                            x.server.clear();
                            x.state = "PENDING";
                        });
                        giaoJob(j.id);
                    } else {
                        suaJob(j.id, [&](JobCtl& x) {
                            x.state = "FAILED";
                            x.reason = "mat ket noi, het so lan giao";
                        });
                        ghiNhatKy("JOB THAT BAI | " + j.id + " | mat ket noi, het so lan giao");
                    }
                }
                continue;
            }

            suaJob(j.id, [&](JobCtl& x) { x.matKetNoiTu = 0; });

            if (r.value("status", "") != "ok") {
                if (r.value("message", "") == "Job not found") {
                    suaJob(j.id, [&](JobCtl& x) {
                        x.state = "FAILED";
                        x.reason = "agent khong con job (agent khoi dong lai?)";
                    });
                    ghiNhatKy("JOB THAT BAI | " + j.id + " | agent khong con job");
                }
                continue;
            }

            std::string st = r.value("State", "");
            if (st == "DONE" || st == "FAILED" || st == "CANCELLED") {
                int ma = r.value("ExitCode", 0);
                suaJob(j.id, [&](JobCtl& x) { x.state = st; });
                if (st != j.state) {
                    ghiNhatKy("JOB KET THUC | " + j.id + " | " + j.server + " | " + st + " | exit " + std::to_string(ma));
                }
            } else if (st == "SUSPENDED") {
                time_t tu = j.dungTu ? j.dungTu : now;
                suaJob(j.id, [&](JobCtl& x) {
                    x.state = "SUSPENDED";
                    if (!x.dungTu) x.dungTu = now;
                });
                if (now - tu >= NGUONG_DUNG_CHUYEN_GIAY && j.lanThu < MAX_LAN_THU) {
                    KetQuaChon kq = chonServer(j.memoryMb, j.daThu);   // chi chuyen neu co server khac phu hop
                    if (kq.idx >= 0) {
                        json huy;
                        std::string l2;
                        hoiAgent(s, "CancelJob", j.agentJobId, huy, l2);
                        suaJob(j.id, [&](JobCtl& x) {
                            x.reason = "chuyen tu " + x.server + " (bi tam dung qua " + std::to_string(NGUONG_DUNG_CHUYEN_GIAY) + "s)";
                            x.server.clear();
                            x.state = "PENDING";
                        });
                        ghiNhatKy("CHUYEN JOB | " + j.id + " | tu " + j.server + " | " + kq.lyDo);
                        giaoJob(j.id);
                    }
                }
            } else if (st == "RUNNING") {
                suaJob(j.id, [&](JobCtl& x) {
                    x.state = "RUNNING";
                    x.dungTu = 0;
                });
            }
        }
        sleep(chuKyGiay);
    }
}