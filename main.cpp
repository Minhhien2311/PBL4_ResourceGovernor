#include "monitor.h"
#include "tcp_server.h"
#include "heartbeat.h"
#include <thread>
#include <csignal>

int main() {
    // Bỏ qua tín hiệu SIGPIPE: nếu client (Controller/Web) ngắt kết nối đột ngột,
    // lệnh send() của Agent sẽ trả về lỗi EPIPE thay vì làm sập toàn bộ tiến trình Agent.
    signal(SIGPIPE, SIG_IGN);

    std::thread luongTcp(chayTcpServer);
    std::thread luongMonitor(chayMonitorLoop);
    std::thread heartbeatThread(chayHeartbeatLoop);

    luongTcp.join();
    luongMonitor.join();
    heartbeatThread.join();
    return 0;
}