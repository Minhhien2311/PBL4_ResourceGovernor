
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
#include <string>
#include <vector>
#include <iomanip>
#include <sstream>

// Thêm thư viện OpenSSL để băm HMAC
#include <openssl/hmac.h>
#include <openssl/crypto.h>

using json = nlohmann::json;

static const size_t MAX_LENH = 8192;

// --- KHU VỰC HÀM BẢO MẬT HMAC ---

// Hàm tính chữ ký HMAC-SHA256
static bool tinhHmacSha256(const std::string& payload, const std::string& secret, std::string& chuKyHex) {
    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int digestLen = 0;
    unsigned char* ketQua = HMAC(EVP_sha256(), secret.data(), static_cast<int>(secret.size()),
                                 reinterpret_cast<const unsigned char*>(payload.data()), payload.size(),
                                 digest, &digestLen);
    if (ketQua == nullptr || digestLen != 32) return false;
    
    std::ostringstream ss;
    ss << std::hex << std::setfill('0');
    for (unsigned int i = 0; i < digestLen; ++i) {
        ss << std::setw(2) << static_cast<unsigned int>(digest[i]);
    }
    chuKyHex = ss.str();
    return true;
}

// Hàm so sánh chữ ký an toàn (chống timing attack)
static bool chuKyBangNhauHangDinh(const std::string& a, const std::string& b) {
    if (a.size() != b.size()) return false;
    return CRYPTO_memcmp(a.data(), b.data(), a.size()) == 0;
}

// --- KẾT THÚC KHU VỰC BẢO MẬT ---


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
    o["AgeSeconds"] = static_cast<long>(time(nullptr) - j.taoLuc);

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
            o["LastSeenSeconds"] = s.daTungKetNoi ? static_cast<long>(time(nullptr) - s.lanCuoi) : -1L;
            o["Note"] = s.loiCuoi;
            arr.push_back(o);
        }
        r["status"] = "ok";
        r["Servers"] = arr;

    } else if (cmd == "GetProcesses") {
        // WebAdmin hoi danh sach tien trinh cua mot server; du lieu do poller keo san tu Agent
        std::string nodeId = j.value("NodeId", "");
        int idx = timServer(nodeId);

        if (idx < 0) return loi("Unknown NodeId");

        ServerInfo s = layBanSao()[idx];
        std::string st = trangThai(s);

        if (st == "CHUA KET NOI" || st == "OFFLINE") {
            std::string msg = "Server " + nodeId + " " + st;
            if (!s.loiCuoi.empty()) msg += ": " + s.loiCuoi;
            return loi(msg);
        }

        json arr = json::array();
        for (const TienTrinh& t : s.dsTienTrinh) {
            json o;
            o["Name"] = t.ten;
            o["Pid"] = t.pid;
            o["User"] = t.user;
            o["RamMB"] = t.ramMb;
	    o["CpuPercent"] = t.cpuAvailable? json(t.cpuPercent): json(nullptr);
            arr.push_back(o);
        }
        r["status"] = "ok";
        r["NodeId"] = nodeId;
        r["State"] = st;
        r["Processes"] = arr;

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
        yc["MaxCpuPercent"] = j.value("MaxCpuPercent", 0);
        yc["ReclaimRam"] = j.value("ReclaimRam", false);

        std::string ph, l;
        if (!guiLenh(s.ip, s.port, yc.dump(), ph, l, 3, s.token)) {
            r = loi("Agent unreachable: " + l);
        } else {
            try { r = json::parse(ph); } catch (...) { r = loi("Bad agent reply"); }
        }
        ghiNhatKy(nguon + " | SetPolicy | " + nodeId + " | " + r.value("status", "?"));

    } else if (cmd == "SetNetworkPolicy") {
        std::string nodeId = j.value("NodeId", "");
        int idx = timServer(nodeId);

        if (idx < 0) return loi("Unknown NodeId");

        ServerInfo s = layBanSao()[idx];

        json yc;
        yc["CommandType"] = "UpdateNetworkPolicy";
        yc["TargetPort"] = j.value("TargetPort", 80);
        yc["MaxBandwidthKbps"] = j.value("MaxBandwidthKbps", 1024);
        yc["ApplyToIp"] = j.value("ApplyToIp", "All");

        std::string ph, l;
        // Gửi đến port_net (cổng 9002 của Network Proxy)
        if (!guiLenh(s.ip, s.port_network, yc.dump(), ph, l, 3, s.token)) {
            r = loi("Proxy unreachable: " + l);
        } else {
            try { r = json::parse(ph); } catch (...) { r = loi("Bad proxy reply"); }
        }
        ghiNhatKy(nguon + " | SetNetworkPolicy | " + nodeId + " | " + r.value("status", "?"));
    } else if (cmd == "RemoveOsPolicy") {
        std::string nodeId = j.value("NodeId", "");
        int idx = timServer(nodeId);
        if (idx < 0) return loi("Unknown NodeId");
        ServerInfo s = layBanSao()[idx];
        json yc;
        yc["CommandType"] = "RemoveOsPolicy";
        yc["TargetUser"] = j.value("TargetUser", "");
        yc["TargetProcess"] = j.value("TargetProcess", "");
        std::string ph, l;
        if (!guiLenh(s.ip, s.port, yc.dump(), ph, l, 3, s.token)) {
            r = loi("Agent unreachable: " + l);
        } else {
            try { r = json::parse(ph); } catch (...) { r = loi("Bad agent reply"); }
        }
        ghiNhatKy(nguon + " | RemovePolicy | " + nodeId + " | " + r.value("status", "?"));
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

        giaoJob(id);

        JobCtl j2;
        if (!layJob(id, j2)) return loi("Cannot read created job");

        r = jobSangJson(j2);
        r["status"] = "ok";
        ghiNhatKy(nguon + " | CreateJob | " + id + " | " + j2.state + " | " + j2.server);

    } else if (cmd == "GetJob") {
        JobCtl jb;
        if (!layJob(j.value("JobId", ""), jb)) return loi("Job not found");
        r = jobSangJson(jb);
        r["status"] = "ok";

    } else if (cmd == "GetJobOutput") {
        std::string id = j.value("JobId", "");
        std::string filename = j.value("Filename", "result.json");
        
        JobCtl jb;
        if (!layJob(id, jb)) return loi("Job not found");
        if (jb.server.empty() || jb.agentJobId.empty()) return loi("Job not assigned to any server yet");
        
        int idx = timServer(jb.server);
        if (idx < 0) return loi("Unknown server");
        
        ServerInfo s = layBanSao()[idx];
        
        json yc;
        yc["CommandType"] = "GetJobOutput";
        yc["JobId"] = jb.agentJobId;
        yc["Filename"] = filename;
        
        std::string ph, l;
        if (!guiLenh(s.ip, s.port, yc.dump(), ph, l, 5, s.token)) {
            r = loi("Agent unreachable: " + l);
        } else {
            try { r = json::parse(ph); } catch (...) { r = loi("Bad agent reply"); }
        }
        ghiNhatKy(nguon + " | GetJobOutput | " + id + " | " + filename + " | " + r.value("status", "?"));

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
        } else if (jb.state == "CANCELLING") {
            // Yeu cau huy da duoc chap nhan; tracker tiep tuc doi Agent xac nhan.
        } else if (jb.state == "SUBMITTING") {
            return loi("Job submission is still being confirmed; retry cancellation shortly");
        } else if (jb.state == "RUNNING" || jb.state == "SUSPENDED" || jb.state == "UNKNOWN") {
            int idx = timServer(jb.server);
            if (idx < 0) return loi("Unknown server");

            ServerInfo s = layBanSao()[idx];
            json yc;
            yc["CommandType"] = "CancelJob";
            yc["JobId"] = jb.agentJobId;

            std::string ph, l;
            if (!guiLenh(s.ip, s.port, yc.dump(), ph, l, 3, s.token)) return loi("Agent unreachable: " + l);

            std::string stateAgent;
            try {
                json phJson = json::parse(ph);
                if (phJson.value("status", "") != "ok") {
                    return loi("Agent rejected CancelJob: " + phJson.value("message", "unknown error"));
                }
                stateAgent = phJson.value("State", "");
            } catch (...) {
                return loi("Bad agent reply for CancelJob");
            }

            suaJob(id, [&](JobCtl& x) {
                if (x.state == "DONE" || x.state == "FAILED" || x.state == "CANCELLED") return;
                if (stateAgent == "DONE" || stateAgent == "FAILED" || stateAgent == "CANCELLED") {
                    x.state = stateAgent;
                    x.reason = "Agent xac nhan job da ket thuc";
                } else {
                    x.state = "CANCELLING";
                    x.reason = "Agent da nhan yeu cau huy; dang doi tien trinh ket thuc";
                }
            });
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

    sockaddr_in addr {};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(cong);

    if (bind(serverfd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        perror("bind"); close(serverfd); return;
    }

    if (listen(serverfd, 5) < 0) { perror("listen"); close(serverfd); return; }

    ghiNhatKy("API khoi dong, lang nghe cong " + std::to_string(cong));

    while (true) {
        sockaddr_in cli {};
        socklen_t len = sizeof(cli);
        int fd = accept(serverfd, reinterpret_cast<sockaddr*>(&cli), &len);

        if (fd < 0) {
            if (errno == EINTR) continue;
            continue;
        }

        char ip[INET_ADDRSTRLEN] = {};
        inet_ntop(AF_INET, &cli.sin_addr, ip, sizeof(ip));
        const std::string nguon = ip;

        struct timeval tv {};
        tv.tv_sec = 5;
        tv.tv_usec = 0;
        setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

        std::string du;
        char buf[1024];
        bool quaDai = false;

        while (true) {
            ssize_t n = recv(fd, buf, sizeof(buf), 0);
            if (n < 0 && errno == EINTR) continue;
            if (n <= 0) break;

            du.append(buf, static_cast<size_t>(n));
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

            // =========================================================
            // BẮT ĐẦU PHẦN XÁC THỰC HMAC MỚI (KHÔNG DÙNG AUTOTOKEN THÔ)
            // =========================================================
            try {
                json outer = json::parse(dong);
                json j;
                bool hopLe = false;
                std::string lyDoTuChoi = "Invalid Signature";

                if (outer.contains("Payload") && outer["Payload"].is_string() &&
                    outer.contains("Signature") && outer["Signature"].is_string()) {
                    
                    std::string payloadStr = outer["Payload"].get<std::string>();
                    std::string clientSignature = outer["Signature"].get<std::string>();
                    std::string serverSignature;

                    // Băm Payload bằng ControllerToken
                    if (tinhHmacSha256(payloadStr, token, serverSignature) && 
                        chuKyBangNhauHangDinh(clientSignature, serverSignature)) {
                        
                        // Parse Payload thực sự
                        j = json::parse(payloadStr);
                        
                        // Kiểm tra Timestamp (Chống Replay Attack)
                        if (j.contains("Timestamp") && j["Timestamp"].is_number_integer()) {
                            long long clientTime = j["Timestamp"].get<long long>();
                            long long serverTime = static_cast<long long>(std::time(nullptr));
                            
                            if (std::llabs(serverTime - clientTime) <= 5) {
                                hopLe = true;
                            } else {
                                lyDoTuChoi = "Packet expired";
                            }
                        } else {
                            lyDoTuChoi = "Missing Timestamp";
                        }
                    }
                } else {
                    lyDoTuChoi = "Missing Payload or Signature";
                }

                if (hopLe) {
                    r = xuLyLenh(j, nguon);
                } else {
                    ghiNhatKy(nguon + " | TU CHOI | " + lyDoTuChoi);
                    r = loi(lyDoTuChoi);
                }
            } catch (const std::exception&) {
                ghiNhatKy(nguon + " | TU CHOI | Malformed JSON");
                r = loi("Malformed JSON");
            }
            // =========================================================
            // KẾT THÚC PHẦN XÁC THỰC HMAC
            // =========================================================
        }

        std::string gui = r.dump() + "\n";
        size_t daGui = 0;

        while (daGui < gui.size()) {
            ssize_t n = send(fd, gui.data() + daGui, gui.size() - daGui, MSG_NOSIGNAL);
            if (n < 0 && errno == EINTR) continue;
            if (n <= 0) break;
            daGui += static_cast<size_t>(n);
        }

        close(fd);
    }
}
