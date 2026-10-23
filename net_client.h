#ifndef NET_CLIENT_H
#define NET_CLIENT_H
#include <string>
// Gui mot dong JSON toi agent, doc phan hoi den het dong. Tra true neu nhan duoc phan hoi.
bool guiLenh(const std::string& ip, int port, const std::string& lenhJson,
             std::string& phanHoi, std::string& loi, int timeoutGiay = 3);
#endif