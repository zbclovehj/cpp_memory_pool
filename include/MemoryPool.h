#pragma once
#include "ThreadCache.h"

namespace Memory_Pool
{

    class MemoryPool
    {
    public:
        static void *allocate(size_t size)
        {
            return ThreadCache::getInstance()->allocate(size);
        }

        static void deallocate(void *ptr, size_t size)
        {
            ThreadCache::getInstance()->deallocate(ptr, size);
        }
    };

} // namespace memoryPool