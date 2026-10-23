#include "poller.h"
#include "logger.h"
#include "net_client.h"
#include "registry.h"
#include <nlohmann/json.hpp>
#include <unistd.h>

using json = nlohmann::json;

void chayPoller(int chuKyGiay) {
    while (true) {
        std::vector<ServerInfo> ds = layBanSao();
        for (size_t i = 0; i < ds.size(); i++) {
            json yeuCau;
            yeuCau["CommandType"] = "GetMetrics";
            yeuCau["AuthToken"] = ds[i].token;

            std::string phanHoi, loi;
            if (!guiLenh(ds[i].ip, ds[i].port, yeuCau.dump(), phanHoi, loi)) {
                if (capNhatThatBai(i, loi)) ghiNhatKy("MAT KET NOI | " + ds[i].ten + " | " + loi);
                continue;
            }

            try {
                json j = json::parse(phanHoi);
                if (j.value("status", "") != "ok") {
                    std::string lyDo = "Agent tu choi: " + j.value("message", "khong ro");
                    if (capNhatThatBai(i, lyDo)) ghiNhatKy("TU CHOI | " + ds[i].ten + " | " + lyDo);
                    continue;
                }
                std::string nodeId = j.value("NodeId", "");
                if (nodeId != ds[i].ten) {
                    // Phat hien nham may hoac gia mao: ten khai trong servers.conf khac NodeId agent tra ve
                    std::string lyDo = "NodeId khong khop: dung " + ds[i].ten + ", nhan " + nodeId;
                    if (capNhatThatBai(i, lyDo)) ghiNhatKy("CANH BAO | " + lyDo);
                    continue;
                }
                bool phucHoi = capNhatThanhCong(i, j.value("CPUUsagePercent", 0.0),
                                                j.value("TotalRAM", 0L), j.value("UsedRAM", 0L),
                                                j.value("ProcessCount", 0));
                if (phucHoi) ghiNhatKy("PHUC HOI | " + ds[i].ten);
            } catch (const std::exception& e) {
                if (capNhatThatBai(i, "JSON hong")) ghiNhatKy("LOI | " + ds[i].ten + " | JSON hong: " + e.what());
            }
        }
        sleep(chuKyGiay);
    }
}