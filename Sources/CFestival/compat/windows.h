#pragma once

#include <stdint.h>
#include <unistd.h>

#if defined(__APPLE__)
#include <sys/sysctl.h>
#endif

typedef unsigned long DWORD;
typedef unsigned short WORD;

typedef struct _SYSTEM_INFO {
    DWORD dwNumberOfProcessors;
} SYSTEM_INFO;

typedef struct _MEMORYSTATUSEX {
    DWORD dwLength;
    DWORD dwMemoryLoad;
    uint64_t ullTotalPhys;
    uint64_t ullAvailPhys;
    uint64_t ullTotalPageFile;
    uint64_t ullAvailPageFile;
    uint64_t ullTotalVirtual;
    uint64_t ullAvailVirtual;
    uint64_t ullAvailExtendedVirtual;
} MEMORYSTATUSEX;

static inline void GetSystemInfo(SYSTEM_INFO *info)
{
    if (info == nullptr) {
        return;
    }

    long processors = sysconf(_SC_NPROCESSORS_ONLN);
    info->dwNumberOfProcessors = processors > 0 ? (DWORD)processors : 1;
}

static inline int GlobalMemoryStatusEx(MEMORYSTATUSEX *status)
{
    if (status == nullptr) {
        return 0;
    }

    uint64_t total = 0;
#if defined(__APPLE__)
    uint64_t mem_size = 0;
    size_t length = sizeof(mem_size);
    if (sysctlbyname("hw.memsize", &mem_size, &length, nullptr, 0) == 0) {
        total = mem_size;
    }
#else
    long pages = sysconf(_SC_PHYS_PAGES);
    long page_size = sysconf(_SC_PAGE_SIZE);
    if (pages > 0 && page_size > 0) {
        total = (uint64_t)pages * (uint64_t)page_size;
    }
#endif

    status->ullTotalPhys = total;
    return 1;
}

#define STD_OUTPUT_HANDLE ((DWORD)-11)

static inline void *GetStdHandle(DWORD)
{
    return nullptr;
}

static inline int SetConsoleTextAttribute(void *, WORD)
{
    return 1;
}
