#ifndef JOB_H
#define JOB_H
#include <nlohmann/json.hpp>
#include <string>

nlohmann::json xuLySubmitJob(const nlohmann::json& j, const std::string& nguon);
nlohmann::json xuLyGetJobStatus(const nlohmann::json& j);
nlohmann::json xuLyCancelJob(const nlohmann::json& j, const std::string& nguon);
void thuHoiJob();   // thu hoi tien trinh con da ket thuc, goi dinh ky
#endif