#ifndef QOSCG_H
#define QOSCG_H

bool cgSanSang();                      // cgroup v2 va controller cpu co dung duoc khong
bool cgGioiHanCpu(int pid, int phanTram);   // dat tien trinh vao cgroup rieng, gioi han phanTram % cua mot loi
void cgGo(int pid);                    // go gioi han, tra tien trinh ve cgroup goc
void cgDonDepKhiKhoiDong();            // don cgroup con sot lai tu lan chay truoc

#endif