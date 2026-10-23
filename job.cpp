#include "job.h"
#include "logger.h"
#include <cctype>
#include <cerrno>
#include <csignal>
#include <cstring>
#include <ctime>
#include <fcntl.h>
#include <fstream>
#include <grp.h>
#include <map>
#include <mutex>
#include <pwd.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

using json = nlohmann::json;

static const size_t MAX_JOB_DONG_THOI = 3;
static const size_t MAX_JOB_BANG = 200;
static const size_t MAX_PARAMS = 8;
static const size_t MAX_DO_DAI = 64;

struct JobAgent {
    pid_t pid = -1;
    std::string loai, user, state;   // RUNNING, DONE, FAILED, CANCELLED
    time_t batDau = 0;
    time_t yeuCauHuy = 0;
    int maThoat = 0;
};

static std::mutex m;
static std::map<std::string, JobAgent> bang;

static json loi(const std::string& msg) {
    json r;
    r["status"] = "error";
    r["message"] = msg;
    return r;
}

static bool idHopLe(const std::string& s) {
    if (s.empty() || s.size() > MAX_DO_DAI) return false;
    for (unsigned char c : s) {
        if (!(std::isalnum(c) || c == '-' || c == '_')) return false;
    }
    return true;
}

static bool thamSoHopLe(const std::string& s) {
    if (s.size() > MAX_DO_DAI) return false;
    for (unsigned char c : s) {
        if (c < 32 || c > 126) return false;
    }
    return true;
}

// Tim duong dan chuong trinh trong jobs.conf (dong "ten=duong_dan"). Chi nhan duong dan tuyet doi va file chay duoc
static std::string timDuongDan(const std::string& loai) {
    std::ifstream f("jobs.conf");
    std::string dong;
    while (std::getline(f, dong)) {
        size_t pos = dong.find('=');
        if (pos == std::string::npos || dong[0] == '#') continue;
        if (dong.substr(0, pos) == loai) {
            std::string p = dong.substr(pos + 1);
            while (!p.empty() && (p.back() == ' ' || p.back() == '\r' || p.back() == '\t')) p.pop_back();
            if (!p.empty() && p[0] == '/' && access(p.c_str(), X_OK) == 0) return p;
            return "";
        }
    }
    return "";
}

static bool dangBiDung(pid_t pid) {
    std::ifstream f("/proc/" + std::to_string(pid) + "/status");
    std::string dong;
    while (std::getline(f, dong)) {
        if (dong.compare(0, 6, "State:") == 0) {
            size_t p = dong.find_first_not_of(" \t", 6);
            return p != std::string::npos && dong[p] == 'T';
        }
    }
    return false;
}

void thuHoiJob() {
    std::lock_guard<std::mutex> lg(m);
    time_t now = time(nullptr);
    for (auto& kv : bang) {
        JobAgent& jb = kv.second;
        if (jb.state != "RUNNING") continue;
        int status = 0;
        pid_t kq = waitpid(jb.pid, &status, WNOHANG);
        if (kq == jb.pid) {
            jb.maThoat = WIFEXITED(status) ? WEXITSTATUS(status) : 128 + WTERMSIG(status);
            if (jb.yeuCauHuy) jb.state = "CANCELLED";
            else if (WIFEXITED(status) && WEXITSTATUS(status) == 0) jb.state = "DONE";
            else jb.state = "FAILED";
            ghiNhatKy("JOB KET THUC | " + kv.first + " | " + jb.state + " | exit " + std::to_string(jb.maThoat));
        } else if (kq < 0 && errno == ECHILD) {
            jb.state = "FAILED";
            jb.maThoat = -1;
            ghiNhatKy("JOB MAT | " + kv.first + " | khong con tien trinh con");
        } else if (jb.yeuCauHuy && now - jb.yeuCauHuy >= 5) {
            kill(jb.pid, SIGKILL);   // SIGTERM khong du, ep ket thuc
        }
    }
}

json xuLySubmitJob(const json& j, const std::string& nguon) {
    thuHoiJob();

    std::string id = j.value("JobId", "");
    std::string loai = j.value("JobType", "");
    std::string tenUser = j.value("TargetUser", "");

    if (!idHopLe(id)) return loi("Invalid JobId");
    if (!idHopLe(loai)) return loi("Invalid JobType");

    std::vector<std::string> thamSo;
    if (j.contains("Params")) {
        if (!j["Params"].is_array() || j["Params"].size() > MAX_PARAMS) return loi("Invalid Params");
        for (const auto& p : j["Params"]) {
            if (!p.is_string() || !thamSoHopLe(p.get<std::string>())) return loi("Invalid Params");
            thamSo.push_back(p.get<std::string>());
        }
    }

    std::string duongDan = timDuongDan(loai);
    if (duongDan.empty()) {
        ghiNhatKy(nguon + " | TU CHOI | SubmitJob JobType la: " + loai);
        return loi("Unknown JobType");
    }

    struct passwd* pw = tenUser.empty() ? nullptr : getpwnam(tenUser.c_str());
    if (pw == nullptr || pw->pw_uid < 1000) {
        ghiNhatKy(nguon + " | TU CHOI | SubmitJob user khong hop le: " + tenUser);
        return loi("Unknown or forbidden user");
    }
    // Lay het thong tin tu pw truoc khi fork: trong tien trinh con khong duoc goi ham cap phat bo nho
    uid_t uid = pw->pw_uid;
    gid_t gid = pw->pw_gid;
    int soNhom = 32;
    std::vector<gid_t> nhom(soNhom);
    if (getgrouplist(pw->pw_name, gid, nhom.data(), &soNhom) < 0) {
        nhom.resize(soNhom);
        getgrouplist(pw->pw_name, gid, nhom.data(), &soNhom);
    }
    nhom.resize(soNhom);

    std::vector<std::string> args;
    args.push_back(duongDan);
    for (const auto& t : thamSo) args.push_back(t);
    std::vector<char*> argv;
    for (auto& s : args) argv.push_back(const_cast<char*>(s.c_str()));
    argv.push_back(nullptr);

    std::lock_guard<std::mutex> lg(m);

    auto it = bang.find(id);
    if (it != bang.end()) {   // goi lai cung JobId: khong chay lai, chi tra trang thai (idempotent)
        json r;
        r["status"] = "ok";
        r["message"] = "Already submitted";
        r["JobId"] = id;
        r["State"] = it->second.state;
        return r;
    }

    size_t dangChay = 0;
    for (const auto& kv : bang) if (kv.second.state == "RUNNING") dangChay++;
    if (dangChay >= MAX_JOB_DONG_THOI) return loi("Too many running jobs");

    if (bang.size() >= MAX_JOB_BANG) {
        for (auto i2 = bang.begin(); i2 != bang.end();) {
            if (i2->second.state != "RUNNING") i2 = bang.erase(i2);
            else ++i2;
        }
        if (bang.size() >= MAX_JOB_BANG) return loi("Job table full");
    }

    pid_t pid = fork();
    if (pid < 0) return loi("Launch failed");

    if (pid == 0) {
        // Tien trinh con: chi dung cac loi goi he thong
        setsid();
        struct rlimit rl;
        rl.rlim_cur = 0;
        rl.rlim_max = 0;
        setrlimit(RLIMIT_CORE, &rl);
        for (int fd = 3; fd < 1024; fd++) close(fd);   // khong ke thua socket cua agent
        int dn = open("/dev/null", O_RDWR);
        if (dn >= 0) {
            dup2(dn, 0);
            dup2(dn, 1);
            dup2(dn, 2);
            if (dn > 2) close(dn);
        }
        if (setgroups(nhom.size(), nhom.data()) != 0) _exit(126);
        if (setgid(gid) != 0) _exit(126);
        if (setuid(uid) != 0) _exit(126);
        if (setuid(0) == 0) _exit(126);   // neu van lay lai duoc root thi khong chay
        if (chdir("/tmp") != 0) _exit(126);
        execv(argv[0], argv.data());
        _exit(127);
    }

    JobAgent jb;
    jb.pid = pid;
    jb.loai = loai;
    jb.user = tenUser;
    jb.state = "RUNNING";
    jb.batDau = time(nullptr);
    bang[id] = jb;

    ghiNhatKy(nguon + " | OK | SubmitJob " + id + " " + loai + " user " + tenUser + " pid " + std::to_string(pid));

    json r;
    r["status"] = "ok";
    r["JobId"] = id;
    r["State"] = "RUNNING";
    r["Pid"] = (int)pid;
    return r;
}

json xuLyGetJobStatus(const json& j) {
    thuHoiJob();
    std::string id = j.value("JobId", "");
    std::lock_guard<std::mutex> lg(m);
    auto it = bang.find(id);
    if (it == bang.end()) return loi("Job not found");

    const JobAgent& jb = it->second;
    std::string st = jb.state;
    if (st == "RUNNING" && dangBiDung(jb.pid)) st = "SUSPENDED";   // monitor dang tam dung job nay

    json r;
    r["status"] = "ok";
    r["JobId"] = id;
    r["JobType"] = jb.loai;
    r["TargetUser"] = jb.user;
    r["State"] = st;
    r["Pid"] = (int)jb.pid;
    r["ElapsedSeconds"] = (long)(time(nullptr) - jb.batDau);
    if (jb.state != "RUNNING") r["ExitCode"] = jb.maThoat;
    return r;
}

json xuLyCancelJob(const json& j, const std::string& nguon) {
    std::string id = j.value("JobId", "");
    std::lock_guard<std::mutex> lg(m);
    auto it = bang.find(id);
    if (it == bang.end()) return loi("Job not found");

    JobAgent& jb = it->second;
    json r;
    r["status"] = "ok";
    r["JobId"] = id;
    if (jb.state != "RUNNING") {
        r["message"] = "Already finished";
        r["State"] = jb.state;
        return r;
    }
    kill(jb.pid, SIGTERM);
    kill(jb.pid, SIGCONT);   // neu job dang bi tam dung thi phai tha ra thi no moi nhan duoc SIGTERM
    jb.yeuCauHuy = time(nullptr);
    ghiNhatKy(nguon + " | OK | CancelJob " + id);
    r["message"] = "Cancel requested";
    r["State"] = "RUNNING";
    return r;
}