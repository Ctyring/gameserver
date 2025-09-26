#pragma once

#pragma pack(push, 1)
#include "cfl.h"
namespace cfl{
    const std::uint8_t CODE_VALUE = 0x12;
    struct PacketHeader {
        std::uint8_t check_code;
        std::uint32_t msg_id;
        std::uint32_t size;
        std::uint32_t packet_id;
        std::uint64_t target_id;
    };
}
#pragma pack(pop)