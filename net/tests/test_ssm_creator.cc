#include "cfl/shm/shm.h"
#include <iostream>
#include <string>

int main() {
    constexpr std::int32_t moduleId = 1;
    constexpr std::int32_t page = 1;
    constexpr std::int32_t size = 1024;

    auto shmHandleOpt = cfl::shm::CreateShareMemory(moduleId, page, size);
    if (!shmHandleOpt) {
        std::cerr << "Failed to create shared memory (already exists?)" << std::endl;
        return 1;
    }

    HANDLE shmHandle = *shmHandleOpt;

    // 映射共享内存
    void* pMem = MapViewOfFile(shmHandle, FILE_MAP_ALL_ACCESS, 0, 0, size);
    if (!pMem) {
        std::cerr << "MapViewOfFile failed: " << GetLastError() << std::endl;
        CloseHandle(shmHandle);
        return 1;
    }

    std::string message = "Hello from creator!";
    memcpy(pMem, message.c_str(), message.size() + 1);

    std::cout << "Creator wrote: " << message << std::endl;
    std::cout << "Press Enter to exit (memory will remain until system restart)" << std::endl;
    std::cin.get();

    UnmapViewOfFile(pMem);
    CloseHandle(shmHandle);
    return 0;
}
