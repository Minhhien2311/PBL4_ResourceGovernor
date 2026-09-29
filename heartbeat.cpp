#include "heartbeat.h"
#include "shared.h"
#include <iostream>
#include <string>
#include <unistd.h>
#include <curl/curl.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// Hàm callback bỏ qua phản hồi trả về từ server để tránh in rác ra màn hình
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    return size * nmemb;
}

void chayHeartbeatLoop() {
    // Khởi tạo global cho curl (chỉ cần chạy 1 lần)
    curl_global_init(CURL_GLOBAL_ALL);

    const std::string url = "http://10.85.188.7:5247/api/telemetry/heartbeat";

    while (true) {
        CURL* curl = curl_easy_init();
        if (curl) {
            // Chuẩn bị dữ liệu JSON
            json bodyJson;
            bodyJson["NodeType"] = "OSAgent_Linux";
            bodyJson["Status"] = "Running";
            bodyJson["CPUUsagePercent"] = 15.5; // Bạn có thể map chỉ số thật từ /proc/stat nếu cần
            bodyJson["TotalRAM"] = 16384;
            bodyJson["UsedRAM"] = 4096;
            bodyJson["ProcessCount"] = 120;

            std::string jsonBody = bodyJson.dump();

            // Thiết lập HTTP Header
            struct curl_slist* headers = nullptr;
            headers = curl_slist_append(headers, "Content-Type: application/json");

            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonBody.c_str());
            
            // Đặt thời gian timeout (5 giây) phòng khi server không phản hồi
            curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);

            // Tắt in body phản hồi ra console
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);

            // Gửi HTTP POST request
            CURLcode res = curl_easy_perform(curl);
            if (res == CURLE_OK) {
                long response_code;
                curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
                std::cout << "[Heartbeat] Da gui heartbeat thanh cong! Ma tra ve: " << response_code << std::endl;
            } else {
                std::cerr << "[Heartbeat] Gui heartbeat that bai: " << curl_easy_strerror(res) << std::endl;
            }

            // Dọn dẹp tài nguyên mỗi lượt gửi
            curl_slist_free_all(headers);
            curl_easy_cleanup(curl);
        } else {
            std::cerr << "[Heartbeat] Khong the khoi tao CURL!" << std::endl;
        }

        // Chờ 5 giây trước khi gửi lần kế tiếp
        sleep(5);
    }

    curl_global_cleanup();
}