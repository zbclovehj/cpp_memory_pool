#pragma once
#include "Common.h"
#include <mutex>
namespace Memory_Pool
{
    class CentralCache
    {
    private:
        // 自由链表 内存操作的原子性
        std::array<std::atomic<void *>, FREE_LIST_SIZE> centralFreeList_;
        // 自旋锁
        std::array<std::atomic_flag, FREE_LIST_SIZE> lock_;
        /* data */
    public:
        // 单例模式
        static CentralCache &getInstance()
        {
            static CentralCache instance;
            return instance;
        }
        void *fetchRange(size_t index, size_t batchNum);
        void returnRange(void *start, size_t size, size_t bytes);

    private:
        CentralCache()
        {
            for (auto &ptr : centralFreeList_)
            {
                ptr.store(nullptr, std::memory_order_relaxed);
            }
            for (auto &lock : lock_)
            {
                lock.clear();
            }
        }
        void *fetchFromPageCache(size_t size);
    };

}