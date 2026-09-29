#include "monitor.h"
#include "tcp_server.h"
#include "heartbeat.h"
#include <thread>

int main() {
    std::thread luongTcp(chayTcpServer);
    std::thread luongMonitor(chayMonitorLoop);
    std::thread heartbeatThread(chayHeartbeatLoop);

    luongTcp.join();
    luongMonitor.join();
    heartbeatThread.join();
    return 0;
}