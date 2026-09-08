#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <tlhelp32.h>   // chứa các hàm/struct Toolhelp32 (Snapshot, PROCESSENTRY32)
#include <iostream>
#include <string>
#include <psapi.h>
#include <thread>
#include <map>
#include <mutex>
#include "json.hpp"
#include <ctime>
#include <fstream>
#include <winhttp.h>
#pragma comment(lib, "psapi.lib")
#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "winhttp.lib")

using json = nlohmann::json;

struct Policy {
    double MaxRAMMB;
    std::wstring Action;
};

std::map <std::wstring, Policy> policyStore;
std::mutex PolicyMutex;
std::mutex Logmutex;

std::wstring StringtoWString(const std::string& s) {
    return std::wstring(s.begin(), s.end());
}

void WriteLog(const std::string& logMessage) {
    time_t now = time(0);
    char timeStr[26];
    ctime_s(timeStr, sizeof(timeStr), &now);
    {
        std::lock_guard<std::mutex> lock(Logmutex);
        std::ofstream logFile("agent_log.txt", std::ios::app);
        logFile << timeStr << " - " << logMessage << std::endl;
    }

}

double GetProcessRAMMB(DWORD pid) {
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
    if (hProcess == NULL) {
        return -1;
    }

    PROCESS_MEMORY_COUNTERS_EX pmc;
    if (GetProcessMemoryInfo(hProcess, (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc))) {
        CloseHandle(hProcess);
        return pmc.WorkingSetSize / 1024.0 / 1024.0;
    }
    CloseHandle(hProcess);
    return -1;
}

void MonitorLoop() {
    while (true) {
        // Tạo snapshot mới ở mỗi vòng lặp để quét tiến trình cập nhật
        HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (hSnapshot == INVALID_HANDLE_VALUE) {
            std::cerr << "Khong tao duoc snapshot!" << std::endl;
            Sleep(5000);
            continue;
        }

        PROCESSENTRY32 entry;
        entry.dwSize = sizeof(PROCESSENTRY32);

        if (Process32First(hSnapshot, &entry)) {
            do {
                double ramMB = GetProcessRAMMB(entry.th32ProcessID);

                std::wcout << L"PID: " << entry.th32ProcessID
                    << L" - Ten: " << entry.szExeFile;
                if (ramMB >= 0) {
                    std::wcout << L" - Ram: " << ramMB << L" MB" << std::endl;
                }
                else {
                    std::wcout << L" - Ram: Khong the lay thong tin RAM" << std::endl;
                }

                Policy policy;
                bool hasPolicy = false;
                {
                    std::lock_guard<std::mutex> lock(PolicyMutex);
                    auto it = policyStore.find(entry.szExeFile);
                    if (it != policyStore.end()) {
                        policy = it->second;
                        hasPolicy = true;
                    }
                }
                if (hasPolicy && ramMB > policy.MaxRAMMB) {
                    std::wcout << L"    >>> VUOT NGUONG!" << std::endl;
                    if (policy.Action == L"Kill") {
                        HANDLE hKill = OpenProcess(PROCESS_TERMINATE, FALSE, entry.th32ProcessID);
                        if (hKill != NULL) {
                            TerminateProcess(hKill, 1);
                            WriteLog("Da kill 1 process vi vuot nguong RAM");
                            CloseHandle(hKill);
                        }
                    }
                }
            } while (Process32Next(hSnapshot, &entry));
        }

        // Đóng handle của snapshot hiện tại sau khi quét xong
        CloseHandle(hSnapshot);
        Sleep(5000); // Ngủ 5 giây trước khi quét lại
    }
}

void TcpServerLoop() {
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        std::cerr << "WSAStartup that bai!" << std::endl;
        return;
    }

    SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSocket == INVALID_SOCKET) {
        std::cerr << "Khong tao duoc socket!" << std::endl;
        WSACleanup();
        return;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(9001);
    int bindResult = bind(listenSocket, (sockaddr*)&serverAddr, sizeof(serverAddr));
    if (bindResult == SOCKET_ERROR) {
        std::cerr << "Bind that bai!" << std::endl;
        closesocket(listenSocket);
        WSACleanup();
        return;
    }

    int listenResult = listen(listenSocket, SOMAXCONN);
    if (listenResult == SOCKET_ERROR) {
        std::cerr << "Listen that bai!" << std::endl;
        closesocket(listenSocket);
        WSACleanup();
        return;
    }

    while (true) {
        SOCKET clientSocket = accept(listenSocket, NULL, NULL);
        if (clientSocket == INVALID_SOCKET) {
            std::cerr << "Accept that bai!" << std::endl;
            continue;
        }
        char buffer[4097];
        int bytesReceived = recv(clientSocket, buffer, 4096, 0);
        if (bytesReceived <= 0) {
            if (bytesReceived == SOCKET_ERROR) {
                WriteLog("Recv that bai tu client");
            }
            closesocket(clientSocket);
            continue;
        }
        buffer[bytesReceived] = '\0';
        try {
            std::string jsonText(buffer);
            json j = json::parse(jsonText);
            std::string targetProcess = j["TargetProcess"];
            double maxRam = j["MaxRamMB"];
            std::string action = j["Action"];

            {
                std::lock_guard<std::mutex> lock(PolicyMutex);
                policyStore[StringtoWString(targetProcess)] = { maxRam, StringtoWString(action) };
            }

            std::cout << "Da cap nhat policy cho: " << targetProcess << std::endl;

            std::string ackText = R"({"status": "ok", "message": "Policy applied"})";
            send(clientSocket, ackText.c_str(), (int)ackText.length(), 0);

        }
        catch (...) {
            std::cerr << "JSON khong hop le!" << std::endl;
            WriteLog("Nhan duoc JSON khong hop le tu client");
        }
        closesocket(clientSocket);
    }

}


void HeartbeatLoop() {
    HINTERNET hSession = WinHttpOpen(L"OSAgent Heartbeat",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) {
        WriteLog("WinHttpOpen that bai");
        return;
    }
    while (true) {
        HINTERNET hConnect = WinHttpConnect(hSession, L"10.85.188.7", 5247, 0);
        if (!hConnect) {
            WriteLog("WinHttpConnect that bai");
            Sleep(5000);
            continue;
        }
        HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"POST", L"/api/telemetry/heartbeat",
            NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, 0);
        if (!hRequest) {
            WriteLog("WinHttpOpenRequest that bai");
            WinHttpCloseHandle(hConnect);
            Sleep(5000);
            continue;
        }
        std::string jsonBody = R"({
            "NodeType": "OSAgent", 
            "Status": "Running", 
            "CPUUsagePercent": 15.5, 
            "TotalRAM": 16384, 
            "UsedRAM": 4096, 
            "ProcessCount": 120
        })";
        LPCWSTR headers = L"Content-Type: application/json";
        BOOL sendResult = WinHttpSendRequest(hRequest,
            headers, -1,
            (LPVOID)jsonBody.c_str(), (DWORD)jsonBody.length(),
            (DWORD)jsonBody.length(), 0);
        if (sendResult) {
            std::cout << "Da gui heartbeat" << std::endl;
        }
        else {
            WriteLog("Gui heartbeat that bai");
        }
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
		Sleep(5000); // Gửi heartbeat mỗi 5 giây
    }
}

int main() {
    std::string jsonText = R"({"CommandType": "UpdateOsPolicy", "TargetProcess": "chrome.exe", "MaxRamMB": 50000.0, "Action": "Kill"})";
    json j = json::parse(jsonText);
    std::string targetProcess = j["TargetProcess"]; 
    double maxRam = j["MaxRamMB"];                     
    std::string action = j["Action"];                   

    policyStore[StringtoWString(targetProcess)] = { maxRam, StringtoWString(action) };

    std::cout << targetProcess << std::endl; 
    std::thread monitorThread(MonitorLoop);
    std::thread tcpThread(TcpServerLoop);
	std::thread heartbeatThread(HeartbeatLoop);
	monitorThread.join();
    tcpThread.join();   
	heartbeatThread.join();
}
