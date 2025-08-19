#include "shmpage.h"

#define BLOCK_CHECK_CODE    0x5A

cfl::shm::SharedMemoryManager::SharedMemoryManager(std::int32_t module_id, std::int32_t raw_block_size,
                                                   std::int32_t blocks_per_page, bool attach_only)
        : blocks_per_page_(blocks_per_page), block_size_(raw_block_size + sizeof(MemoryBlockHeader)),
          raw_block_size_(raw_block_size), module_id_(module_id) {
    std::int32_t nSize = blocks_per_page_ * (block_size_);
    page_count_ = 0;
    total_blocks_ = 0;

    SharedMemoryPage firstpage;
    firstpage.handle = cfl::shm::OpenShareMemory(module_id_, 0);
    if (firstpage.handle.has_value()) {
        firstpage.raw_data = (char *) cfl::shm::GetShareMemory(firstpage.handle);
        if (firstpage.raw_data != nullptr) {
            firstpage.block_headers = (MemoryBlockHeader *) (firstpage.raw_data + raw_block_size_ * page_count_);
            pages_.push_back(firstpage);
            import_existing_pages();
            empty_created_ = false;
        } else {
            return;
        }
    } else {
        if (!attach_only) {
            firstpage.handle = cfl::shm::CreateShareMemory(module_id_, 0, nSize);
            if (!firstpage.handle.has_value()) {
                return;
            }

            firstpage.raw_data = (char *) cfl::shm::GetShareMemory(firstpage.handle);

            ///找到头数据块的头
            firstpage.block_headers = (MemoryBlockHeader *) (firstpage.raw_data + raw_block_size_ * blocks_per_page_);

            ///清空所有内存;
            memset(firstpage.block_headers, 0, nSize);

            page_count_++;
            total_blocks_ += blocks_per_page_;
            init_page(firstpage);

            pages_.push_back(firstpage);
        }

        empty_created_ = true;
    }
}

void cfl::shm::SharedMemoryManager::import_existing_pages() {
    while (1) {
        SharedMemoryPage page;
        page.handle = cfl::shm::OpenShareMemory(module_id_, page_count_);
        if (!page.handle.has_value()) {
            break;
        }
        page.raw_data = (CHAR *) cfl::shm::GetShareMemory(page.handle);
        if (page.raw_data == nullptr) {
            break;
        }
        page.block_headers = (MemoryBlockHeader *) (page.raw_data + raw_block_size_ * page_count_);
        pages_.push_back(page);
        total_blocks_ += blocks_per_page_;
        page_count_++;
    }
}

void cfl::shm::SharedMemoryManager::init_page(SharedMemoryPage &page) {
//    char *pdata = page.raw_data;
    std::fill_n(page.raw_data, blocks_per_page_ * raw_block_size_, 0);
//    for (std::int32_t i = 0; i != blocks_per_page_; ++i) {
//        *(pdata + (block_size_) * i) = BLOCK_CHECK_CODE;
//    }

    auto start_index = blocks_per_page_ * (page_count_ - 1);
    auto headers = std::span<MemoryBlockHeader>(page.block_headers, blocks_per_page_);

    for (std::size_t i = 0; i < headers.size(); ++i) {
        auto& header = headers[i];
        new (&header) MemoryBlockHeader();
        header.index = static_cast<std::int32_t>(start_index + i);

        block_map_.emplace(header.index, &header);
        free_blocks_.emplace(header.index, &header);
    }

//    for (std::int32_t i = nStartindex; i < total_blocks_; ++i) {
//        MemoryBlockHeader *ptem = &(page.block_headers[i - nStartindex]);
//        new(ptem)(MemoryBlockHeader);
//        ptem->index = i;
//        block_map_.insert(std::make_pair(i, ptem));
//        free_blocks_.insert(std::make_pair(i, ptem));
//    }
}
