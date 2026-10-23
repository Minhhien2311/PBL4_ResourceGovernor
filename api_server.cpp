#include "api_server.h"
#include "config.h"
#include "jobstore.h"
#include "logger.h"
#include "net_client.h"
#include "registry.h"
#include "scheduler.h"
#include <arpa/inet.h>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <iostream>
#include <netinet/in.h>
#include <nlohmann/json.hpp>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

using json = nlohmann::json;

static const size_t MAX_LENH = 8192;

static json loi(const std::string& msg) {
    json r;
    r["status"] = "error";
    r["message"] = msg;
    return r;
}

static json jobSangJson(const JobCtl& j) {
    json o;
    o["JobId"] = j.id;
    o["JobType"] = j.loai;
    o["TargetUser"] = j.user;
    o["State"] = j.state;
    o["AssignedTo"] = j.server;
    o["Reason"] = j.reason;
    o["Attempts"] = j.lanThu;
    o["MemoryMB"] = j.memoryMb;
    o["AgeSeconds"] = (long)(time(nullptr) - j.taoLuc);
    return o;
}

static json xuLyLenh(const json& j, const std::string& nguon) {
    std::string cmd = j.value("CommandType", "");
    json r;

    if (cmd == "GetServers") {
        json arr = json::array();
        std::vector<ServerInfo> ds = layBanSao();
        for (const ServerInfo& s : ds) {
            json o;
            o["NodeId"] = s.ten;
            o["Ip"] = s.ip;
            o["State"] = trangThai(s);
            o["CPUUsagePercent"] = s.cpu;
            o["TotalRAM"] = s.tongRamMb;
            o["UsedRAM"] = s.daDungRamMb;
            o["ProcessCount"] = s.soTienTrinh;
            o["LastSeenSeconds"] = s.daTungKetNoi ? (long)(time(nullptr) - s.lanCuoi) : -1L;
            o["Note"] = s.loiCuoi;
            arr.push_back(o);
        }
        r["status"] = "ok";
        r["Servers"] = arr;

    } else if (cmd == "SetPolicy") {
        std::string nodeId = j.value("NodeId", "");
        int idx = timServer(nodeId);
        if (idx < 0) return loi("Unknown NodeId");
        ServerInfo s = layBanSao()[idx];

        json yc;
        yc["CommandType"] = "UpdateOsPolicy";
        yc["TargetUser"] = j.value("TargetUser", "");
        yc["TargetProcess"] = j.value("TargetProcess", "");
        yc["MaxRamMB"] = j.value("MaxRamMB", 0);
        yc["Action"] = "Suspend";
        yc["AuthToken"] = s.token;

        std::string ph, l;
        if (!guiLenh(s.ip, s.port, yc.dump(), ph, l, 3)) {
            r = loi("Agent unreachable: " + l);
        } else {
            try { r = json::parse(ph); } catch (...) { r = loi("Bad agent reply"); }
        }
        ghiNhatKy(nguon + " | SetPolicy | " + nodeId + " | " + r.value("status", "?"));

    } else if (cmd == "CreateJob") {
        std::string loai = j.value("JobType", "");
        std::string user = j.value("TargetUser", "");
        long mem = j.value("MemoryMB", 100L);
        std::vector<std::string> params;
        bool ok = !loai.empty() && loai.size() <= 64 && !user.empty() && user.size() <= 32 && mem >= 1 && mem <= 16384;
        if (j.contains("Params")) {
            if (!j["Params"].is_array() || j["Params"].size() > 8) {
                ok = false;
            } else {
                for (const auto& p : j["Params"]) {
                    if (!p.is_string() || p.get<std::string>().size() > 64) { ok = false; break; }
                    params.push_back(p.get<std::string>());
                }
            }
        }
        if (!ok) return loi("Invalid job request");

        JobCtl mau;
        mau.loai = loai;
        mau.user = user;
        mau.params = params;
        mau.memoryMb = mem;
        std::string id = taoJob(mau);
        if (id.empty()) return loi("Job table full");

        giaoJob(id);   // thu giao ngay, khong duoc thi job nam trong hang doi (PENDING)
        JobCtl j2;
        layJob(id, j2);
        r = jobSangJson(j2);
        r["status"] = "ok";
        ghiNhatKy(nguon + " | CreateJob | " + id + " | " + j2.state + " | " + j2.server);

    } else if (cmd == "GetJob") {
        JobCtl jb;
        if (!layJob(j.value("JobId", ""), jb)) return loi("Job not found");
        r = jobSangJson(jb);
        r["status"] = "ok";

    } else if (cmd == "ListJobs") {
        json arr = json::array();
        for (const JobCtl& jb : layTatCa()) arr.push_back(jobSangJson(jb));
        r["status"] = "ok";
        r["Jobs"] = arr;

    } else if (cmd == "CancelJob") {
        std::string id = j.value("JobId", "");
        JobCtl jb;
        if (!layJob(id, jb)) return loi("Job not found");

        if (jb.state == "PENDING") {
            suaJob(id, [&](JobCtl& x) { x.state = "CANCELLED"; x.reason = "huy khi dang cho"; });
        } else if (jb.state == "RUNNING" || jb.state == "SUSPENDED") {
            int idx = timServer(jb.server);
            if (idx < 0) return loi("Unknown server");
            ServerInfo s = layBanSao()[idx];
            json yc;
            yc["CommandType"] = "CancelJob";
            yc["JobId"] = jb.agentJobId;
            yc["AuthToken"] = s.token;
            std::string ph, l;
            if (!guiLenh(s.ip, s.port, yc.dump(), ph, l, 3)) return loi("Agent unreachable: " + l);
            suaJob(id, [&](JobCtl& x) { x.state = "CANCELLED"; x.reason = "bi huy theo yeu cau"; });
        }
        layJob(id, jb);
        r = jobSangJson(jb);
        r["status"] = "ok";
        ghiNhatKy(nguon + " | CancelJob | " + id);

    } else {
        ghiNhatKy(nguon + " | TU CHOI | Lenh khong ho tro: " + cmd);
        return loi("Unknown command");
    }
    return r;
}

void chayApiServer() {
    const std::string token = docCauHinh("Token", "ctl-token");
    const int cong = std::atoi(docCauHinh("Port", "9100").c_str());

    int serverfd = socket(AF_INET, SOCK_STREAM, 0);
    if (serverfd < 0) { perror("socket"); return; }
    int opt = 1;
    setsockopt(serverfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(cong);
    if (bind(serverfd, (sockaddr*)&addr, sizeof(addr)) < 0) { perror("bind"); close(serverfd); return; }
    if (listen(serverfd, 5) < 0) { perror("listen"); close(serverfd); return; }
    ghiNhatKy("API khoi dong, lang nghe cong " + std::to_string(cong));

    while (true) {
        sockaddr_in cli{};
        socklen_t len = sizeof(cli);
        int fd = accept(serverfd, (sockaddr*)&cli, &len);
        if (fd < 0) continue;

        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &cli.sin_addr, ip, sizeof(ip));
        const std::string nguon = ip;

        struct timeval tv;
        tv.tv_sec = 5;
        tv.tv_usec = 0;
        setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

        std::string du;
        char buf[1024];
        bool quaDai = false;
        while (true) {
            ssize_t n = recv(fd, buf, sizeof(buf), 0);
            if (n <= 0) break;
            du.append(buf, n);
            if (du.size() > MAX_LENH) { quaDai = true; break; }
            if (du.find('\n') != std::string::npos) break;
            if (json::accept(du)) break;
        }

        json r;
        if (quaDai) {
            ghiNhatKy(nguon + " | TU CHOI | Lenh qua dai");
            r = loi("Message too large");
        } else if (du.empty()) {
            close(fd);
            continue;
        } else {
            size_t pos = du.find('\n');
            std::string dong = (pos == std::string::npos) ? du : du.substr(0, pos);
            try {
                json j = json::parse(dong);
                if (j.contains("AuthToken") && j["AuthToken"] == token) {
                    r = xuLyLenh(j, nguon);
                } else {
                    ghiNhatKy(nguon + " | TU CHOI | Sai token");
                    r = loi("Invalid token");
                }
            } catch (const std::exception& e) {
                ghiNhatKy(nguon + " | TU CHOI | Malformed JSON");
                r = loi("Malformed JSON");
            }
        }

        std::string gui = r.dump() + "\n";
        send(fd, gui.c_str(), gui.size(), MSG_NOSIGNAL);
        close(fd);
    }
}