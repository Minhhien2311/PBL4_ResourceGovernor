#include "jobstore.h"
#include <cstdio>
#include <map>
#include <mutex>

static std::mutex m;
static std::map<std::string, JobCtl> bang;
static int demId = 0;
static const size_t MAX_JOB = 500;

std::string taoJob(JobCtl mau) {
    std::lock_guard<std::mutex> lg(m);
    if (bang.size() >= MAX_JOB) return "";
    char buf[16];
    std::snprintf(buf, sizeof(buf), "job-%04d", ++demId);
    mau.id = buf;
    mau.taoLuc = time(nullptr);
    bang[mau.id] = mau;
    return mau.id;
}

bool layJob(const std::string& id, JobCtl& out) {
    std::lock_guard<std::mutex> lg(m);
    auto it = bang.find(id);
    if (it == bang.end()) return false;
    out = it->second;
    return true;
}

std::vector<JobCtl> layTatCa() {
    std::lock_guard<std::mutex> lg(m);
    std::vector<JobCtl> kq;
    for (const auto& kv : bang) kq.push_back(kv.second);
    return kq;
}

bool suaJob(const std::string& id, const std::function<void(JobCtl&)>& f) {
    std::lock_guard<std::mutex> lg(m);
    auto it = bang.find(id);
    if (it == bang.end()) return false;
    f(it->second);
    return true;
}