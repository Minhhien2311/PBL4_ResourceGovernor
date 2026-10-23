#ifndef JOBSTORE_H
#define JOBSTORE_H
#include <ctime>
#include <functional>
#include <string>
#include <vector>

struct JobCtl {
    std::string id, loai, user;
    std::vector<std::string> params;
    long memoryMb = 0;
    std::string server;         // server dang giu job, rong neu PENDING
    std::string agentJobId;     // id cua job tren agent (job-0001-1)
    std::string state = "PENDING";   // PENDING, RUNNING, SUSPENDED, DONE, FAILED, CANCELLED
    std::string reason;
    int lanThu = 0;
    time_t taoLuc = 0, giaoLuc = 0, dungTu = 0, matKetNoiTu = 0;
    std::vector<std::string> daThu;  // cac server da giao truoc do
};

std::string taoJob(JobCtl mau);   // tra ve id, rong neu bang day
bool layJob(const std::string& id, JobCtl& out);
std::vector<JobCtl> layTatCa();
bool suaJob(const std::string& id, const std::function<void(JobCtl&)>& f);
#endif