#include "qoscg.h"
#include "logger.h"
#include <cctype>
#include <cerrno>
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <fstream>
#include <string>
#include <sys/stat.h>
#include <sys/vfs.h>
#include <unistd.h>
#include <vector>

static const std::string GOC = "/sys/fs/cgroup";
static const std::string QOS = "/sys/fs/cgroup/qos";
static bool daThu = false;
static bool sanSang = false;

static bool ghiFile(const std::string& duongDan, const std::string& noiDung) {
    int fd = open(duongDan.c_str(), O_WRONLY);
    if (fd < 0) return false;
    ssize_t n = write(fd, noiDung.c_str(), noiDung.size());
    close(fd);
    return n == (ssize_t)noiDung.size();
}

bool cgSanSang() {
    if (daThu) return sanSang;
    daThu = true;

    struct statfs st;
    if (statfs(GOC.c_str(), &st) != 0 || st.f_type != 0x63677270) {   // CGROUP2_SUPER_MAGIC
        ghiNhatKy("CGROUP | khong dung cgroup v2, bo qua gioi han CPU");
        return false;
    }

    std::ifstream f(GOC + "/cgroup.controllers");
    std::string dong;
    std::getline(f, dong);
    if (dong.find("cpu") == std::string::npos) {
        ghiNhatKy("CGROUP | khong co controller cpu");
        return false;
    }

    ghiFile(GOC + "/cgroup.subtree_control", "+cpu");   // thuong da bat san, loi thi bo qua
    mkdir(QOS.c_str(), 0755);                           // da co thi thoi
    if (!ghiFile(QOS + "/cgroup.subtree_control", "+cpu")) {
        ghiNhatKy(std::string("CGROUP | khong bat duoc controller cpu cho qos | ") + std::strerror(errno));
        return false;
    }

    sanSang = true;
    ghiNhatKy("CGROUP | san sang gioi han CPU");
    return true;
}

bool cgGioiHanCpu(int pid, int phanTram) {
    if (!cgSanSang()) return false;
    std::string dir = QOS + "/p" + std::to_string(pid);
    mkdir(dir.c_str(), 0755);

    long quota = (long)phanTram * 1000;      // chu ky 100000 us: 100% = 1 loi
    if (quota < 1000) quota = 1000;
    if (!ghiFile(dir + "/cpu.max", std::to_string(quota) + " 100000")) return false;
    return ghiFile(dir + "/cgroup.procs", std::to_string(pid));
}

void cgGo(int pid) {
    if (!sanSang) return;
    ghiFile(GOC + "/cgroup.procs", std::to_string(pid));   // tien trinh da thoat thi loi nay bo qua duoc
    rmdir((QOS + "/p" + std::to_string(pid)).c_str());
}

void cgDonDepKhiKhoiDong() {
    if (!cgSanSang()) return;
    DIR* d = opendir(QOS.c_str());
    if (d == nullptr) return;

    std::vector<std::string> con;
    struct dirent* e;
    while ((e = readdir(d)) != nullptr) {
        std::string n = e->d_name;
        bool dung = n.size() > 1 && n[0] == 'p';
        for (size_t i = 1; dung && i < n.size(); i++) {
            if (!isdigit(static_cast<unsigned char>(n[i]))) dung = false;
        }
        if (dung) con.push_back(n);     // chi lay thu muc dang p<so>, bo qua file nhu pids.current
    }
    closedir(d);

    for (const std::string& n : con) {
        std::string dir = QOS + "/" + n;
        std::ifstream f(dir + "/cgroup.procs");
        std::string pid;
        while (std::getline(f, pid)) ghiFile(GOC + "/cgroup.procs", pid);
        rmdir(dir.c_str());
    }
}