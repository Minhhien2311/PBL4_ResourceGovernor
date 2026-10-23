#include "net_client.h"
#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

bool guiLenh(const std::string& ip, int port, const std::string& lenhJson,
             std::string& phanHoi, std::string& loi, int timeoutGiay) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) { loi = std::string("socket: ") + std::strerror(errno); return false; }

    struct timeval tv;
    tv.tv_sec = timeoutGiay;
    tv.tv_usec = 0;
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));   // cung gioi han thoi gian cho connect

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (inet_pton(AF_INET, ip.c_str(), &addr.sin_addr) != 1) {
        loi = "IP khong hop le: " + ip;
        close(fd);
        return false;
    }
    if (connect(fd, (sockaddr*)&addr, sizeof(addr)) < 0) {
        loi = std::string("connect: ") + std::strerror(errno);
        close(fd);
        return false;
    }

    std::string gui = lenhJson + "\n";
    size_t daGui = 0;
    while (daGui < gui.size()) {
        ssize_t n = send(fd, gui.data() + daGui, gui.size() - daGui, MSG_NOSIGNAL);
        if (n <= 0) {
            loi = std::string("send: ") + std::strerror(errno);
            close(fd);
            return false;
        }
        daGui += n;
    }

    phanHoi.clear();
    char buf[1024];
    while (phanHoi.find('\n') == std::string::npos && phanHoi.size() < 65536) {
        ssize_t n = recv(fd, buf, sizeof(buf), 0);
        if (n <= 0) break;
        phanHoi.append(buf, n);
    }
    close(fd);

    if (phanHoi.empty()) { loi = "khong co phan hoi"; return false; }
    size_t pos = phanHoi.find('\n');
    if (pos != std::string::npos) phanHoi.resize(pos);
    return true;
}