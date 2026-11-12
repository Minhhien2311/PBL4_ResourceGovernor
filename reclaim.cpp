#include "reclaim.h"
#include <cstdio>
#include <fstream>
#include <string>
#include <sys/syscall.h>
#include <sys/uio.h>
#include <unistd.h>

#ifndef MADV_PAGEOUT
#define MADV_PAGEOUT 21
#endif
#ifndef SYS_pidfd_open
#define SYS_pidfd_open 434
#endif
#ifndef SYS_process_madvise
#define SYS_process_madvise 440
#endif

long thuHoiRamTienTrinh(int pid) {
    int pidfd = (int)syscall(SYS_pidfd_open, pid, 0);
    if (pidfd < 0) return -1;

    std::ifstream maps("/proc/" + std::to_string(pid) + "/maps");
    std::string dong;
    long tong = 0;
    while (std::getline(maps, dong)) {
        unsigned long dau = 0, cuoi = 0;
        char quyen[8] = {0};
        if (std::sscanf(dong.c_str(), "%lx-%lx %7s", &dau, &cuoi, quyen) != 3) continue;
        if (dong.find("[vsyscall]") != std::string::npos) continue;
        if (dong.find("[vvar]") != std::string::npos) continue;
        if (dong.find("[vdso]") != std::string::npos) continue;

        struct iovec iov;
        iov.iov_base = reinterpret_cast<void*>(dau);
        iov.iov_len = cuoi - dau;
        // Moi vung nho mot lan goi: vung loi (khong ep duoc) khong lam dung cac vung con lai
        long r = syscall(SYS_process_madvise, pidfd, &iov, 1UL, MADV_PAGEOUT, 0U);
        if (r > 0) tong += r;
    }
    close(pidfd);
    return tong;
}