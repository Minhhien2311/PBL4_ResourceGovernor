#ifndef NET_CLIENT_H
#define NET_CLIENT_H

#include <string>

bool guiLenh(
    const std::string& ip,
    int port,
    const std::string& lenhJson,
    std::string& phanHoi,
    std::string& loi,
    int timeoutGiay,
    const std::string& secretToken
);

#endif