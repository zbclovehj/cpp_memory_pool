#include "../include/CentralCache.h"
#include "../include/PageCache.h"
#include <cassert>
#include <thread>
namespace Memory_Pool
{
    // 从pagecache获取的页的大小
    static const size_t SPAN_PAGES = 8;
    void *CentralCache::fetchRange(size_t index, size_t batchNum)
    {
        // 索引检查，当索引大于等于FREE_LIST_SIZE时，说明申请内存过大应直接向系统申请
        if (index >= FREE_LIST_SIZE || batchNum == 0)
            return nullptr;

        // 自旋锁保护
        while (lock_[index].test_and_set(std::memory_order_acquire))
        {
            std::this_thread::yield(); // 添加线程让步，避免忙等待，避免过度消耗CPU
        }

        void *result = nullptr;
        try
        {
            // 尝试从中心缓存获取内存块
            result = centralFreeList_[index].load(std::memory_order_relaxed);

            if (!result)
            {
                // 如果中心缓存为空，从页缓存获取新的内存块
                size_t size = (index + 1) * ALIGNMENT;
                result = fetchFromPageCache(size);

                if (!result)
                {
                    lock_[index].clear(std::memory_order_release);
                    return nullptr;
                }

                // 将从PageCache获取的内存块切分成小块
                char *start = static_cast<char *>(result);
                size_t totalBlocks = (SPAN_PAGES * PageCache::PAGE_SIZE) / size;
                size_t allocBlocks = std::min(batchNum, totalBlocks);

                // 构建返回给ThreadCache的内存块链表
                if (allocBlocks > 1)
                {
                    // 确保至少有两个块才构建链表
                    // 构建链表
                    for (size_t i = 1; i < allocBlocks; ++i)
                    {
                        void *current = start + (i - 1) * size;
                        void *next = start + i * size;
                        *reinterpret_cast<void **>(current) = next;
                    }
                    *reinterpret_cast<void **>(start + (allocBlocks - 1) * size) = nullptr;
                }

                // 构建保留在CentralCache的链表
                if (totalBlocks > allocBlocks)
                {
                    void *remainStart = start + allocBlocks * size;
                    for (size_t i = allocBlocks + 1; i < totalBlocks; ++i)
                    {
                        void *current = start + (i - 1) * size;
                        void *next = start + i * size;
                        *reinterpret_cast<void **>(current) = next;
                    }
                    *reinterpret_cast<void **>(start + (totalBlocks - 1) * size) = nullptr;

                    centralFreeList_[index].store(remainStart, std::memory_order_release);
                }
            }
            else // 如果中心缓存有index对应大小的内存块
            {
                // 从现有链表中获取指定数量的块
                void *current = result;
                void *prev = nullptr;
                size_t count = 0;

                while (current && count < batchNum)
                {
                    prev = current;
                    current = *reinterpret_cast<void **>(current);
                    count++;
                }

                if (prev) // 当前centralFreeList_[index]链表上的内存块大于batchNum时需要用到
                {
                    *reinterpret_cast<void **>(prev) = nullptr;
                }

                centralFreeList_[index].store(current, std::memory_order_release);
            }
        }
        catch (...)
        {
            lock_[index].clear(std::memory_order_release);
            throw;
        }

        // 释放锁
        lock_[index].clear(std::memory_order_release);
        return result;
    }
    void CentralCache::returnRange(void *start, size_t size, size_t index)
    {
        // 归还的索引大于链表时，表示大内存直接归还系统
        if (!start || index > FREE_LIST_SIZE)
        {
            return;
        }
        while (lock_[index].test_and_set(std::memory_order_acquire))
        {
            std::this_thread::yield();
        }
        try
        {
            void *current = centralFreeList_[index].load(std::memory_order_relaxed);
            *reinterpret_cast<void **>(start) = current;
        }
        catch (...)
        {
            lock_[index].clear(std::memory_order_release);
            throw;
        }
        lock_[index].clear(std::memory_order_relaxed);
    }
    void *CentralCache::fetchFromPageCache(size_t size)
    {
        size_t numPages = (size + PageCache::PAGE_SIZE - 1) / PageCache::PAGE_SIZE;
        if (size <= SPAN_PAGES * PageCache::PAGE_SIZE)
        {
            // 小于等于32KB的请求，使用固定8页
            return PageCache::getInstance().allocateSpan(SPAN_PAGES);
        }
        else
        {
            return PageCache::getInstance().allocateSpan(numPages);
        }
    }
}