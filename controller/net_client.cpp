
#include "net_client.h"

#include <arpa/inet.h>
#include <algorithm>
#include <cerrno>
#include <cstring>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

#include <nlohmann/json.hpp>
#include <openssl/evp.h>
#include <openssl/hmac.h>

using json = nlohmann::json;

static const size_t MAX_REQUEST_BYTES = 8192;
static const size_t MAX_RESPONSE_BYTES = 16 * 1024 * 1024;

static bool tinhHMAC(
    const std::string& payload,
    const std::string& secret,
    std::string& chuKy
) {
    if (secret.empty()) {
        return false;
    }

    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int digestLen = 0;

    unsigned char* ketQua = HMAC(
        EVP_sha256(),
        secret.data(),
        static_cast<int>(secret.size()),
        reinterpret_cast<const unsigned char*>(payload.data()),
        payload.size(),
        digest,
        &digestLen
    );

    if (ketQua == nullptr || digestLen != 32) {
        return false;
    }

    std::ostringstream ss;
    ss << std::hex << std::setfill('0');

    for (unsigned int i = 0; i < digestLen; ++i) {
        ss << std::setw(2)
           << static_cast<unsigned int>(digest[i]);
    }

    chuKy = ss.str();
    return true;
}

bool guiLenh(
    const std::string& ip,
    int port,
    const std::string& lenhJson,
    std::string& phanHoi,
    std::string& loi,
    int timeoutGiay,
    const std::string& secretToken
) {
    phanHoi.clear();
    loi.clear();

    if (secretToken.empty()) {
        loi = "Secret token rong";
        return false;
    }

    if (timeoutGiay <= 0) {
        loi = "Timeout khong hop le";
        return false;
    }

    // 1. Parse lenh JSON goc.
    json payloadJson;

    try {
        payloadJson = json::parse(lenhJson);
    } catch (const std::exception&) {
        loi = "JSON lenh khong hop le";
        return false;
    }

    if (!payloadJson.is_object()) {
        loi = "Payload phai la JSON object";
        return false;
    }

    // 2. Them timestamp vao payload.
    payloadJson["Timestamp"] =
        static_cast<long long>(std::time(nullptr));

    // 3. Chuoi payload chinh xac duoc dung de tinh HMAC.
    const std::string payload = payloadJson.dump();

    // 4. Ky payload bang secret cua Agent dich.
    std::string signature;

    if (!tinhHMAC(payload, secretToken, signature)) {
        loi = "Khong the tinh HMAC-SHA256";
        return false;
    }

    // 5. Dong goi theo giao thuc HMAC cua Agent.
    json envelope;
    envelope["Payload"] = payload;
    envelope["Signature"] = signature;

    const std::string duLieuGui = envelope.dump() + "\n";
    if (duLieuGui.size() > MAX_REQUEST_BYTES) {
        loi = "Request vuot gioi han 8192 byte cua Agent";
        return false;
    }

    // 6. Tao socket.
    int fd = socket(AF_INET, SOCK_STREAM, 0);

    if (fd < 0) {
        loi = std::string("socket: ") + std::strerror(errno);
        return false;
    }

    struct timeval tv {};
    tv.tv_sec = timeoutGiay;
    tv.tv_usec = 0;

    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

    sockaddr_in addr {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    if (inet_pton(AF_INET, ip.c_str(), &addr.sin_addr) != 1) {
        loi = "IP khong hop le: " + ip;
        close(fd);
        return false;
    }

    if (connect(
            fd,
            reinterpret_cast<sockaddr*>(&addr),
            sizeof(addr)
        ) < 0) {
        loi = std::string("connect: ") + std::strerror(errno);
        close(fd);
        return false;
    }

    // 7. Gui het du lieu, ke ca khi send chi gui mot phan.
    size_t daGui = 0;

    while (daGui < duLieuGui.size()) {
        ssize_t n = send(
            fd,
            duLieuGui.data() + daGui,
            duLieuGui.size() - daGui,
            MSG_NOSIGNAL
        );

        if (n < 0 && errno == EINTR) {
            continue;
        }

        if (n <= 0) {
            loi = std::string("send: ") + std::strerror(errno);
            close(fd);
            return false;
        }

        daGui += static_cast<size_t>(n);
    }

    // 8. Doc phan hoi den het dong.
    char buf[1024];
    bool nhanDuDong = false;

    while (phanHoi.find('\n') == std::string::npos &&
           phanHoi.size() < MAX_RESPONSE_BYTES) {
        const size_t conLai = MAX_RESPONSE_BYTES - phanHoi.size();
        const size_t kichThuocDoc = std::min(sizeof(buf), conLai);
        ssize_t n = recv(fd, buf, kichThuocDoc, 0);

        if (n < 0 && errno == EINTR) {
            continue;
        }

        if (n <= 0) {
            break;
        }

        phanHoi.append(buf, static_cast<size_t>(n));
        nhanDuDong = phanHoi.find('\n') != std::string::npos;
    }

    close(fd);

    if (!nhanDuDong) {
        loi = phanHoi.size() >= MAX_RESPONSE_BYTES
            ? "Response vuot gioi han 16 MiB"
            : (phanHoi.empty() ? "Khong co phan hoi" : "Response bi cat truoc newline/timeout");
        phanHoi.clear();
        return false;
    }

    size_t pos = phanHoi.find('\n');

    if (pos != std::string::npos) {
        phanHoi.resize(pos);
    }

    return true;
}
