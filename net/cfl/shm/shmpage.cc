#include "shmpage.h"
#include "spdlog/spdlog.h"
#include <cstring>
#include <span>
#define BLOCK_CHECK_CODE 0x5A

#ifndef _WIN32
/// @brief Linux/Unix 下定义 INVALID_HANDLE_VALUE
#define INVALID_HANDLE_VALUE (-1)
#endif

using namespace cfl::shm;

/**
 * @brief 构造函数
 * @param module_id 模块编号
 * @param raw_block_size 每个原始块大小（字节数，不含头部）
 * @param blocks_per_page 每页的块数
 * @param attach_only 是否仅附加到已有共享内存
 *
 * @details
 * - 如果指定的共享内存已存在，则尝试附加并导入已有页。
 * - 如果共享内存不存在且 attach_only=false，则创建新页并初始化。
 */
SharedMemoryManagerBase::SharedMemoryManagerBase(std::size_t module_id, std::size_t raw_block_size,
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
            spdlog::error("SharedMemoryManagerBase::SharedMemoryManagerBase: module_id = {}", module_id_);
            firstpage.block_headers = (MemoryBlockHeader *) (firstpage.raw_data + raw_block_size_ * page_count_);
            pages_.emplace_back(std::move(firstpage));
            import_existing_pages();
            empty_created_ = false;
        } else {
            spdlog::error("SharedMemoryManagerBase::SharedMemoryManagerBase: firstpage.raw_data == nullptr");
            return;
        }
    } else {
        if (!attach_only) {
            firstpage.handle = CreateShareMemory(module_id_, 0, nSize);
            if (!firstpage.handle.has_value()) {
                spdlog::error("CreateShareMemory failed: module_id = {}, nSize = {}", module_id_, nSize);
                return;
            }

            firstpage.raw_data = (char *) GetShareMemory(firstpage.handle);
            firstpage.block_headers = (MemoryBlockHeader *) (firstpage.raw_data + raw_block_size_ * blocks_per_page_);

            /// 清空所有内存
            memset(firstpage.raw_data, 0, nSize);

            page_count_++;
            total_blocks_ += blocks_per_page_;
//            init_page(firstpage);

            pages_.emplace_back(firstpage);
            init_page(pages_.back());
        }
        empty_created_ = true;
    }
}

/**
 * @brief 析构函数
 * @details 释放所有已映射的共享内存，并关闭句柄
 */
SharedMemoryManagerBase::~SharedMemoryManagerBase() {
    for (std::size_t r = 0; r < (std::size_t)pages_.size(); r++) {
        cfl::shm::ReleaseShareMemory(pages_[r].raw_data);
        cfl::shm::CloseShareMemory(pages_[r].handle);

        spdlog::error("SharedMemoryManagerBase::~SharedMemoryManagerBase: module_id = {}", module_id_);
        pages_[r].handle = INVALID_HANDLE_VALUE;
        pages_[r].raw_data = nullptr;
    }
    pages_.clear();
}

/**
 * @brief 导入现有共享内存页
 * @details 递归打开 module_id 下的所有共享内存页，并加入页列表
 */
void SharedMemoryManagerBase::import_existing_pages() {
    while (auto handle = OpenShareMemory(module_id_, page_count_)) {
        SharedMemoryPage page;
        page.handle = std::move(handle);
        page.raw_data = static_cast<char*>(GetShareMemory(page.handle));
        if (!page.raw_data) break;

        page.block_headers = reinterpret_cast<MemoryBlockHeader*>(
                page.raw_data + raw_block_size_ * blocks_per_page_);
        pages_.push_back(std::move(page));
        ++page_count_;
        total_blocks_ += blocks_per_page_;
    }
}

/**
 * @brief 初始化一个共享内存页
 * @param page 待初始化的页
 *
 * @details
 * - 将页面数据区清零
 * - 初始化所有块头
 * - 注册到 block_map_ 和 free_blocks_ 中
 */
void SharedMemoryManagerBase::init_page(SharedMemoryPage &page) {
    volatile char test = *page.raw_data; // 触发页访问
//    std::fill_n(page.raw_data, blocks_per_page_ * raw_block_size_, 0);

    auto start_index = blocks_per_page_ * (page_count_ - 1);
    auto headers = std::span<MemoryBlockHeader>(page.block_headers, blocks_per_page_);
    for (std::size_t i = 0; i < headers.size(); ++i) {
        auto& header = headers[i];
        new(&header) MemoryBlockHeader();
        header.index = static_cast<std::size_t>(start_index + i);

        block_map_.emplace(header.index, &header);
        free_blocks_.insert({header.index, &header});
    }
}

/**
 * @brief 分配一个新对象
 * @param new_block 是否标记为新建
 * @return 分配成功返回对象指针，否则返回空
 *
 * @details
 * - 优先从空闲块中分配
 * - 如果没有空闲块，则清理已释放的块
 * - 如果仍然不足，尝试创建新页
 */
std::optional<SharedObject *> SharedMemoryManagerBase::allocate_object(bool new_block) {
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
        auto* header = it->second;
        SharedObject *pObject = get_object(header->index);
        if (pObject == nullptr) {
            ++it;
            continue;
        }

        if (!pObject->is_destroyed()) {
            used_blocks_.insert(std::make_pair(pObject, std::ref(header)));
            free_blocks_.erase(it);
            header->in_use = true;
            header->is_new = new_block;

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

MemoryBlockHeader *SharedMemoryManagerBase::get_block_header(std::size_t index){
    if (index >= total_blocks_) {
        return nullptr;
    }
    auto which_page = index / blocks_per_page_;
    auto page_index = index % blocks_per_page_;
    auto& page = pages_[which_page];
    return &page.block_headers[page_index];
}

/**
 * @brief 根据索引获取对象指针
 * @param index 块索引
 * @return 指向共享对象的指针，越界时返回 nullptr
 */
SharedObject *SharedMemoryManagerBase::get_object(std::size_t index) {
    if (index >= total_blocks_) {
        return nullptr;
    }

    std::size_t whichPage = index / blocks_per_page_;
    std::size_t pageIndex = index % blocks_per_page_;
    SharedMemoryPage &page = pages_[whichPage];
    return reinterpret_cast<SharedObject *>(page.raw_data + raw_block_size_ * pageIndex);
}

/**
 * @brief 销毁一个对象
 * @param obj 待销毁的对象
 * @return 是否成功销毁
 *
 * @details
 * - 将对象重置
 * - 将其从 used_blocks_ 移动到 free_blocks_
 */
bool SharedMemoryManagerBase::destroy_object(cfl::shm::SharedObject *obj) {
    if (obj == nullptr) {
        return false;
    }
    obj->reset();
    auto it = used_blocks_.find(obj);
    if (it == used_blocks_.end()) {
        return false;
    }
    auto header = it->second;
    header->in_use = false;
    used_blocks_.erase(it);
    free_blocks_.insert(std::make_pair(header->index, std::ref(header)));
    return true;
}

/**
 * @brief 清理已用区块中未被使用的对象
 *
 * @details
 * 遍历 used_blocks_，将已经被释放的对象重新放入 free_blocks_
 */
void SharedMemoryManagerBase::clean_dirty_blocks() {
    std::erase_if(used_blocks_, [&](auto& kv) {
        auto* pObject = static_cast<SharedObject *>(kv.first);
        auto& header = kv.second;
        if (!pObject->is_in_use()) {
            pObject->reset();
            header->in_use = false;
            free_blocks_.insert({header->index, std::ref(header)});
            return true; // 删除
        }
        return false;
    });
}

/**
 * @brief 创建一个新页
 * @return 是否创建成功
 *
 * @details
 * - 调用 CreateShareMemory 新建共享内存
 * - 初始化页数据
 * - 加入页列表
 */
bool SharedMemoryManagerBase::create_new_page() {
    std::size_t nSize = blocks_per_page_ * (block_size_);
    SharedMemoryPage newPage;
    spdlog::info("create new page %d", page_count_);
    newPage.handle = CreateShareMemory(module_id_, page_count_, nSize);
    spdlog::info("new page handle %d", newPage.handle.value());
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
//    init_page(newPage);
    pages_.emplace_back(std::move(newPage));
    init_page(pages_.back());
    return true;
}

void SharedMemoryManagerBase::initialize_block_map(){
    if(!empty_created_){
        for(std::size_t i = 0; i < total_blocks_; i++){
            auto header = get_block_header(i);
            auto obj = get_object(i);
            if(header->in_use && (obj->state() == ObjectState::InUse || obj->state() == ObjectState::Locked)){
                used_blocks_.insert(std::make_pair(obj, header));
            }
            else{
                free_blocks_.insert(std::make_pair(i, header));
            }
            block_map_.insert(std::make_pair(i, header));
        }
    }
    else{
        if(pages_.empty()){
            page_count_ = 0;
            return;
        }

        init_page(pages_[0]);
    }
}