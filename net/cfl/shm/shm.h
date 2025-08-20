#pragma once

#include <string>
#include <optional>
#include <cstdint>
#include <system_error>
#include <stdexcept>
#include <format>

#if defined(_WIN32)

#include <windows.h>

#else
#include <sys/ipc.h>
#include <sys/shm.h>
#include <cerrno>
#endif

namespace cfl::shm {

#if defined(_WIN32)
    using ShmHandle = HANDLE;
#else
    using ShmHandle = int; // shmget 返回 int 标识符
#endif

    /// 创建共享内存
    inline std::optional<ShmHandle> CreateShareMemory(std::size_t moduleId, std::size_t page, std::size_t size) {
#if defined(_WIN32)
        auto name = std::format("SM_{}", (moduleId << 16) | page);
        HANDLE hShare = CreateFileMappingA(
                INVALID_HANDLE_VALUE,
                nullptr,
                PAGE_READWRITE,
                0,
                static_cast<DWORD>(size),
                name.c_str()
        );
        if (!hShare) {
            return std::nullopt;
        }
        if (GetLastError() == ERROR_ALREADY_EXISTS) {
            CloseHandle(hShare);
            return std::nullopt;
        }
        return hShare;
#else
        int key = (moduleId << 16) | page;
        int hShare = shmget(key, size, 0666 | IPC_CREAT | IPC_EXCL);
        if (hShare == -1) {
            return std::nullopt;
        }
        return hShare;
#endif
    }

    /// 打开共享内存
    inline std::optional<ShmHandle> OpenShareMemory(std::size_t moduleId, std::size_t page) {
#if defined(_WIN32)
        auto name = std::format("SM_{}", (moduleId << 16) | page);
        HANDLE hShare = OpenFileMappingA(FILE_MAP_READ | FILE_MAP_WRITE, FALSE, name.c_str());
        if (!hShare) {
            return std::nullopt;
        }
        return hShare;
#else
        int key = (moduleId << 16) | page;
        int hShare = shmget(key, 0, 0);
        if (hShare == -1) {
            return std::nullopt;
        }
        return hShare;
#endif
    }

    // 根据句柄获取共享内存地址
    inline char *GetShareMemory(std::optional<ShmHandle> hShm) {
        if (!hShm.has_value()) {
            return nullptr;
        }
#ifdef WIN32
        char *pdata = (char *) MapViewOfFile(hShm.value(), FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, 0);
#else
        char* pdata = (char*)shmat(hShm.value(), (void*)0, 0);
#endif
        return pdata;
    }

    bool ReleaseShareMemory(char *pMem) {
#ifdef WIN32
        return UnmapViewOfFile(pMem);
#else
        return (0 == shmdt(pMem));
#endif
    }

    bool CloseShareMemory(std::optional<ShmHandle> hShm) {
#ifdef WIN32
        return CloseHandle(hShm.value());
#else
        return (0 == shmctl(hShm, IPC_RMID, 0));
#endif
    }

} // namespace cfl::shm
