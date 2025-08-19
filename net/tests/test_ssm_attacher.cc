#include "cfl/shm/shm.h"
#include <iostream>

int main() {
    constexpr std::int32_t moduleId = 1;
    constexpr std::int32_t page = 1;

    auto shmHandleOpt = cfl::shm::OpenShareMemory(moduleId, page);
    if (!shmHandleOpt) {
        std::cerr << "Failed to open shared memory" << std::endl;
        return 1;
    }

    HANDLE shmHandle = *shmHandleOpt;

    // 映射共享内存
    void* pMem = MapViewOfFile(shmHandle, FILE_MAP_ALL_ACCESS, 0, 0, 0);
    if (!pMem) {
        std::cerr << "MapViewOfFile failed: " << GetLastError() << std::endl;
        CloseHandle(shmHandle);
        return 1;
    }

    char* data = static_cast<char*>(pMem);
    std::cout << "Attacher read: " << data << std::endl;

    UnmapViewOfFile(pMem);
    CloseHandle(shmHandle);
    return 0;
}
