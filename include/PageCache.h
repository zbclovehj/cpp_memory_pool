#pragma once
#include "Common.h"
#include <map>
#include <mutex>
namespace Memory_Pool
{
    class PageCache
    {
    public:
        static const size_t PAGE_SIZE = 4096;
        static PageCache &getInstance()
        {
            static PageCache instance;
            return instance;
        }
        void *allocateSpan(size_t numPages);
        void deallocateSpan(void *ptr, size_t numPages);

    private:
        PageCache() = default;
        void *systemAlloc(size_t numPages);

    private:
        struct Span
        {
            void *pageAddr;
            size_t numPages;
            Span *next;
            /* data */
        };
        // 按页数管理空闲span，不同页数对应不同Span链表
        std::map<size_t, Span *> freeSpans_;
        // 管理已分配的内存 指针指向对应的分配的内存信息
        std::map<void *, Span *> spanMap_;
        std::mutex mutex_;
    };
}