#include "shmpage.h"

#define BLOCK_CHECK_CODE    0x5A
using namespace cfl::shm;

SharedMemoryManager::SharedMemoryManager(std::size_t module_id, std::size_t raw_block_size,
                                         std::size_t blocks_per_page, bool attach_only)
        : blocks_per_page_(blocks_per_page), block_size_(raw_block_size + sizeof(MemoryBlockHeader)),
          raw_block_size_(raw_block_size), module_id_(module_id) {
    std::size_t nSize = blocks_per_page_ * (block_size_);
    page_count_ = 0;
    total_blocks_ = 0;

    SharedMemoryPage firstpage;
    firstpage.handle = OpenShareMemory(module_id_, 0);
    if (firstpage.handle.has_value()) {
        firstpage.raw_data = (char *) GetShareMemory(firstpage.handle);
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
            firstpage.handle = CreateShareMemory(module_id_, 0, nSize);
            if (!firstpage.handle.has_value()) {
                return;
            }

            firstpage.raw_data = (char *) GetShareMemory(firstpage.handle);

            ///找到头数据块的头
            firstpage.block_headers = (MemoryBlockHeader *) (firstpage.raw_data + raw_block_size_ * blocks_per_page_);

            ///清空所有内存;
            memset(firstpage.raw_data, 0, nSize);

            page_count_++;
            total_blocks_ += blocks_per_page_;
            init_page(firstpage);

            pages_.push_back(firstpage);
        }

        empty_created_ = true;
    }
}

SharedMemoryManager::~SharedMemoryManager()
{
    for (std::size_t r = 0; r < (std::size_t)pages_.size(); r++)
    {
        cfl::shm::ReleaseShareMemory(pages_[r].raw_data);
        cfl::shm::CloseShareMemory(pages_[r].handle);
        pages_[r].handle = INVALID_HANDLE_VALUE;
        pages_[r].raw_data = nullptr;
    }

    pages_.clear();
}

void SharedMemoryManager::import_existing_pages() {
    while (auto handle = OpenShareMemory(module_id_, page_count_)) {
        SharedMemoryPage page{ .handle = handle.value() };
        page.raw_data = static_cast<char*>(GetShareMemory(page.handle));
        if (!page.raw_data) break;

        page.block_headers = reinterpret_cast<MemoryBlockHeader*>(
                page.raw_data + raw_block_size_ * blocks_per_page_);
        pages_.push_back(std::move(page));
        ++page_count_;
        total_blocks_ += blocks_per_page_;
    }
}

void SharedMemoryManager::init_page(SharedMemoryPage &page) {
//    char *pdata = page.raw_data;
    std::fill_n(page.raw_data, blocks_per_page_ * raw_block_size_, 0);
//    for (std::size_t i = 0; i != blocks_per_page_; ++i) {
//        *(pdata + (block_size_) * i) = BLOCK_CHECK_CODE;
//    }

    auto start_index = blocks_per_page_ * (page_count_ - 1);
    auto headers = std::span<MemoryBlockHeader>(page.block_headers, blocks_per_page_);

    for (std::size_t i = 0; i < headers.size(); ++i) {
        auto &header = headers[i];
        new(&header) MemoryBlockHeader();
        header.index = static_cast<std::size_t>(start_index + i);

        block_map_.emplace(header.index, std::ref(header));
        free_blocks_.emplace(header.index, std::ref(header));
    }

//    for (std::size_t i = nStartindex; i < total_blocks_; ++i) {
//        MemoryBlockHeader *ptem = &(page.block_headers[i - nStartindex]);
//        new(ptem)(MemoryBlockHeader);
//        ptem->index = i;
//        block_map_.insert(std::make_pair(i, ptem));
//        free_blocks_.insert(std::make_pair(i, ptem));
//    }
}

std::optional<SharedObject *> SharedMemoryManager::allocate_object(bool new_block) {
    if (free_blocks_.empty()) {
        clean_dirty_blocks();
    }
    if (free_blocks_.empty()) {
        if (create_new_page()) {
            return allocate_object(new_block);
        } else {
            return nullptr;
        }
    }

    auto it = free_blocks_.begin();
    while (it != free_blocks_.end()) {
        auto &header = it->second.get();
        SharedObject *pObject = get_object(header.index);
        if (pObject == nullptr) {
            ++it;
            continue;
        }

        if (!pObject->is_destroyed()) {
            used_blocks_.insert(std::make_pair(pObject, std::ref(header)));
            free_blocks_.erase(it);
            header.in_use = true;
            header.is_new = new_block;

            pObject->use();

            return pObject;
        }
        it++;
    }

    clean_dirty_blocks();

    if (create_new_page()) {
        return allocate_object(new_block);
    }

    return nullptr;
}

SharedObject *SharedMemoryManager::get_object(std::size_t index) {
    if (index >= total_blocks_) {
        return nullptr;
    }

    std::size_t whichPage = index / blocks_per_page_;
    std::size_t pageIndex = index % blocks_per_page_;
    SharedMemoryPage &page = pages_[whichPage];
    return reinterpret_cast<SharedObject *>(page.raw_data + raw_block_size_ * pageIndex);
}

bool SharedMemoryManager::destroy_object(cfl::shm::SharedObject *obj)
{
    if (obj == nullptr)
    {
        return false;
    }
    obj->reset();
    auto it = used_blocks_.find(obj);
    if (it == used_blocks_.end())
    {
        return false;
    }
    auto& header = it->second.get();
    header.in_use = false;
    used_blocks_.erase(it);
    free_blocks_.insert(std::make_pair(header.index, std::ref(header)));
    return true;
}

void SharedMemoryManager::clean_dirty_blocks() {
    std::erase_if(used_blocks_, [&](auto& kv) {
        auto* pObject = static_cast<SharedObject *>(kv.first);
        auto& header = kv.second.get();
        if (!pObject->is_in_use()) {
            pObject->reset();
            header.in_use = false;
            free_blocks_.insert({header.index, std::ref(header)});
            return true; // 删除
        }
        return false;
    });
}

bool SharedMemoryManager::create_new_page() {
    std::size_t nSize = blocks_per_page_ * (block_size_);
    SharedMemoryPage newPage;
    newPage.handle = CreateShareMemory(module_id_, page_count_, nSize);
    if (!newPage.handle.has_value()) {
        return false;
    }

    newPage.raw_data = GetShareMemory(newPage.handle);
    if (newPage.raw_data == nullptr) {
        return false;
    }

    newPage.block_headers = (MemoryBlockHeader *) (newPage.raw_data + raw_block_size_ * blocks_per_page_);
    page_count_++;
    total_blocks_ += blocks_per_page_;
    init_page(newPage);
    pages_.push_back(newPage);
    return true;
}

