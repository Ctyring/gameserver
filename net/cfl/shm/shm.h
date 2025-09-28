#pragma once

#include <string>
#include <optional>
#include <cstdint>
#include <system_error>
#include <stdexcept>
#include <format>

#if defined(_WIN32)
#define NOMINMAX
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

    inline bool ReleaseShareMemory(char *pMem) {
#ifdef WIN32
        return UnmapViewOfFile(pMem);
#else
        return (0 == shmdt(pMem));
#endif
    }

    inline bool CloseShareMemory(std::optional<ShmHandle> hShm) {
#ifdef WIN32
        return CloseHandle(hShm.value());
#else
        return (0 == shmctl(hShm.value(), IPC_RMID, 0));
#endif
    }

    inline std::size_t get_last_error()
    {
#ifdef WIN32
        return ::GetLastError();
#else
        return errno;
#endif
    }

    inline std::string get_last_error_str(int error_code) {
#ifdef _WIN32
        LPWSTR buffer = nullptr;

        const DWORD size = FormatMessageW(
                FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                nullptr,
                error_code,
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                reinterpret_cast<LPWSTR>(&buffer),
                0,
                nullptr
        );

        // 用 unique_ptr 自动释放 LocalAlloc 内存
        auto deleter = [](LPWSTR p) { if (p) LocalFree(p); };
        std::unique_ptr<wchar_t, decltype(deleter)> msg_ptr(buffer, deleter);

        if (size == 0 || !buffer) {
            return "Unknown error: " + std::to_string(error_code);
        }

        // 转换为 UTF-8 string
        int utf8_size = WideCharToMultiByte(CP_UTF8, 0, buffer, -1, nullptr, 0, nullptr, nullptr);
        std::string result(utf8_size - 1, '\0');
        WideCharToMultiByte(CP_UTF8, 0, buffer, -1, result.data(), utf8_size, nullptr, nullptr);

        return result;
#else
        return std::system_error(error_code, std::generic_category()).what();
#endif
    }

} // namespace cfl::shm
