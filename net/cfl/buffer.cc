#include "buffer.h"

namespace cfl{
    std::shared_ptr<DataBuffer> BufferAllocator::allocate_buffer(int size) {
        spdlog::info("allocate buffer size:{}", size);
        if(size < 64)
        {
            return buffer_manager_64b.allocate_buffer();
        }
        if(size < 128)
        {
            return buffer_manager_128b.allocate_buffer();
        }
        if(size < 256)
        {
            return buffer_manager_256b.allocate_buffer();
        }
        if(size < 512)
        {
            return buffer_manager_512b.allocate_buffer();
        }
        if(size < 1024)
        {
            return buffer_manager_1k.allocate_buffer();
        }
        else if(size < 2048)
        {
            return buffer_manager_2k.allocate_buffer();
        }
        else if(size < 4096)
        {
            return buffer_manager_4k.allocate_buffer();
        }
        else if(size < 8192)
        {
            return buffer_manager_8k.allocate_buffer();
        }
        else if(size < 16384)
        {
            return buffer_manager_16k.allocate_buffer();
        }
        else if(size < 32768)
        {
            return buffer_manager_32k.allocate_buffer();
        }
        else if(size < 65536)
        {
            return buffer_manager_64k.allocate_buffer();
        }

        return buffer_manager_any.allocate_buffer();
    }
}