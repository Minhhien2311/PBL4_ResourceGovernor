#include "heartbeat.h"
#include "config.h"
#include "metrics.h"
#include <curl/curl.h>
#include <iostream>
#include <nlohmann/json.hpp>
#include <string>
#include <unistd.h>

using json = nlohmann::json;

// Bo qua phan hoi tra ve tu server de khong in rac ra man hinh
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    return size * nmemb;
}

void chayHeartbeatLoop() {
    curl_global_init(CURL_GLOBAL_ALL);

    while (true) {
        // Doc cau hinh moi vong lap: sua agent.conf la co hieu luc, khong can bien dich lai
        const std::string url = docCauHinh("HeartbeatUrl", "http://localhost:5247/api/telemetry/heartbeat");
        const std::string nodeId = docCauHinh("NodeId", "server-a");

        SoLieu sl = laySoLieu();

        CURL* curl = curl_easy_init();
        if (curl) {
            json bodyJson;
            bodyJson["NodeType"] = "OSAgent";          // dung theo tai lieu nhom
            bodyJson["NodeId"] = nodeId;               // truong them de phan biet nhieu server
            bodyJson["Status"] = "Running";
            bodyJson["CPUUsagePercent"] = sl.cpuPhanTram;
            bodyJson["TotalRAM"] = sl.tongRamMb;
            bodyJson["UsedRAM"] = sl.daDungRamMb;
            bodyJson["ProcessCount"] = sl.soTienTrinh;

            std::string jsonBody = bodyJson.dump();

            struct curl_slist* headers = nullptr;
            headers = curl_slist_append(headers, "Content-Type: application/json");

            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonBody.c_str());
            curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);
            curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);   // an toan khi chay trong luong rieng
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);

            CURLcode res = curl_easy_perform(curl);
            if (res == CURLE_OK) {
                long response_code = 0;
                curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
                std::cout << "[Heartbeat] OK, ma tra ve: " << response_code << std::endl;
            } else {
                std::cerr << "[Heartbeat] That bai: " << curl_easy_strerror(res) << std::endl;
            }

            curl_slist_free_all(headers);
            curl_easy_cleanup(curl);
        } else {
            std::cerr << "[Heartbeat] Khong the khoi tao CURL!" << std::endl;
        }

        sleep(5);
    }
}