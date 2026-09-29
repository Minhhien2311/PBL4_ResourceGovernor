#include "monitor.h"
#include "shared.h"
#include <dirent.h>
#include <cctype>
#include <fstream>
#include <sstream>
#include <unistd.h>
#include <unordered_set>
#include <signal.h>
#include <sys/types.h>
#include <iostream>

void chayMonitorLoop() {
    std::unordered_set<int> danhsachDaSuspend;
    while (true) {
        std::string tenCanQuanLy;
        long soKbMaxHienTai;
        {
            std::lock_guard<std::mutex> lg(khoa);
            tenCanQuanLy = tenDangQuanLy;
            soKbMaxHienTai = soKbMaxDungChung;
        }

        std::string dd = "/proc/meminfo";
        std::ifstream file(dd);
        if (!file.is_open()){
            std::cout << "khong the mo file" << std::endl;
        }
        std::string dong;
        long memTotal = 0;
        long memAvailable = 0;
        while(std::getline(file, dong)) {
            if (dong.substr(0, 9) == "MemTotal:"){
                    std::istringstream iss(dong); 
                    std::string nhan;
                    std::string donvi;
                    iss >> nhan >> memTotal >> donvi ;
            }
            if (dong.substr(0, 13) == "MemAvailable:"){
                    std::istringstream iss(dong); 
                    std::string nhan; 
                    std::string donvi;
                    iss >> nhan >> memAvailable >> donvi ;
            }
        }
        double phanTramTong = double(memAvailable) / double(memTotal) * 100 ;
        std::cout << "RAM he thong con trong: " << phanTramTong << "%" << std::endl;

        DIR* dir = opendir("/proc");
        if (dir == nullptr){
            std::cerr <<("Loi roi nhe, opendir khong duoc!");
            continue ;
        }
        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr){
            bool laPid = true;
            std::string ten = entry->d_name;
            std::string duongDan = "/proc/" + ten + "/status";
            for(int i = 0; i < ten.length(); i++){
                char c = ten[i];
                if(isdigit(c)){
                }
                else{
                    laPid = false;
                    break;
                }
            }
            if (laPid){
                std::ifstream file(duongDan);
                if (!file.is_open()){
                    std::cout << "khong the mo file" << std::endl;
                }
                std::string dong;
                std::string tenTienTrinh;   
                while (std::getline(file, dong)) {
                    if (dong.substr(0, 5) == "Name:"){
                        std::istringstream iss(dong);
                        std::string nhan;
                        iss >> nhan >> tenTienTrinh;
                    }
                     
                    if (dong.substr(0, 6) == "VmRSS:" && tenTienTrinh == tenCanQuanLy) {
                        std::istringstream iss(dong); 
                        std::string nhan;
                        long soluong; 
                        std::string donvi;
                        iss >> nhan >> soluong >> donvi ;
                        int soPid = std::stoi(ten);   // stoi = "string to integer"
                        if (soluong >= soKbMaxHienTai && danhsachDaSuspend.count(soPid) == 0 && phanTramTong <= 29){
                            int temp = kill(soPid, SIGSTOP);
                            if (temp == 0){
                                danhsachDaSuspend.insert(soPid);
                                std::cout << "Da tam dung thanh cong tien trinh co PID: " << soPid << std::endl;
                            }
                        }
                        if (danhsachDaSuspend.count(soPid) == 1 && phanTramTong >= 32){
                            int cont = kill(soPid, SIGCONT);
                            if (cont == 0){
                                danhsachDaSuspend.erase(soPid);
                                std::cout << "Cho tiep tuc tien trinh co PID: " << soPid << std::endl;
                            }
                        }
                        break;
                    }
                }
                
            }
        }
        sleep(3);
    }
}