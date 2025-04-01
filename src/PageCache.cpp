#include "../include/PageCache.h"
#include <sys/mman.h>
#include <cstring>
namespace Memory_Pool
{
    void *PageCache::allocateSpan(size_t numPages)
    {
        // 互斥锁申请资源
        std::lock_guard<std::mutex> lock(mutex_);
        // 找到大于等于所申请页面的迭代器
        auto it = freeSpans_.lower_bound(numPages);
        if (it != freeSpans_.end())
        {
            Span *span = it->second;
            if (span->next)
            {
                // 指针向后偏移
                freeSpans_[it->first] = span->next;
            }
            else
            {
                // 如果只有这一个span则删除当前页数对应的链表
                freeSpans_.erase(it);
            }
            if (span->numPages > numPages)
            {
                Span *newSpan = new Span;
                // void*的指针是不能进行运算的
                //  转化为char*类型可以将指针指向的地址精确到字节并进行单字节运算，
                //  所以加上我们需要的字节数就是我们要分割位置了
                newSpan->pageAddr = static_cast<char *>(span->pageAddr) + numPages * PAGE_SIZE;
                newSpan->next = nullptr;
                newSpan->numPages = span->numPages - numPages;
                // 将超出部分放回空闲Span*列表头部,采用对应部分的头插法
                auto &list = freeSpans_[newSpan->numPages];
                newSpan->next = list;
                list = newSpan;
                span->numPages = numPages;
            }
            spanMap_[span->pageAddr] = span;
            return span->pageAddr;
        }
        // 通过系统分配numpages的页面大小的内存
        void *memory = systemAlloc(numPages);
        // 判断是否分配成功
        if (!memory)
            return nullptr;
        Span *span = new Span;
        span->next = nullptr;
        span->pageAddr = memory;
        span->numPages = numPages;
        spanMap_[memory] = span;
        return memory;
    }
    void PageCache::deallocateSpan(void *ptr, size_t numPages)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = spanMap_.find(ptr);
        if (it == spanMap_.end())
            return;
        Span *span = it->second;
        // 尝试将需要回收的地址判断是否可以合并
        void *nextAddr = static_cast<char *>(ptr) + numPages * PAGE_SIZE;
        auto nextIt = spanMap_.find(nextAddr);
        // 找到链表说明可以合并
        if (nextIt != spanMap_.end())
        {
            Span *nextSpan = nextIt->second;
            bool found = false;
            // 获取对应页面的头结点
            auto &nextList = freeSpans_[nextSpan->numPages];
            if (nextList == nextSpan)
            {
                // 头结点向后偏移
                nextList = nextSpan->next;
                found = true;
            }
            else if (nextList)
            {
                Span *prev = nextList;
                while (prev->next)
                {
                    if (prev->next == nextSpan)
                    {
                        // 将nextSpan从空闲链表中移除
                        prev->next = nextSpan->next;
                        found = true;
                        break;
                    }
                    prev = prev->next;
                    /* code */
                }
            }
            if (found)
            {
                span->numPages += nextSpan->numPages;
                spanMap_.erase(nextAddr);
                delete nextSpan;
            }
        }
        // 将合并后的span通过头插法插入空闲列表
        auto &list = freeSpans_[span->numPages];
        span->next = list;
        list = span;
    }
    void *PageCache::systemAlloc(size_t numPages)
    {
        size_t size = numPages * PAGE_SIZE;
        void *ptr = mmap(nullptr, size, PROT_READ | PROT_WRITE,
                         MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (ptr == MAP_FAILED)
        {
            return nullptr;
            /* code */
        }
        memset(ptr, 0, size);
        return ptr;
    }
}