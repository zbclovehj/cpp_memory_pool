#pragma once
#include <cstddef>
#include <atomic>
#include <array>
namespace Memory_Pool
{
    constexpr size_t ALIGNMENT = 8;
    constexpr size_t MAX_BYTES = 256 * 1024;
    // 自由链表的长度
    constexpr size_t FREE_LIST_SIZE = MAX_BYTES / ALIGNMENT;
    struct BlockHeader
    {
        size_t size;
        bool inUse;
        BlockHeader *next;
        /* data */
    };
    class SizeClass
    {
    public:
        static size_t roundUp(size_t bytes)
        {
            // 将一个数值 bytes 向上对齐到 ALIGNMENT 的整数倍
            return (bytes + ALIGNMENT - 1) & ~(ALIGNMENT - 1);
        }
        // 获得每个要请求内存比特数的下标
        static size_t getIndex(size_t bytes)
        {
            bytes = std::max(bytes, ALIGNMENT);
            // 块号从0开始
            return (bytes + ALIGNMENT - 1) / ALIGNMENT - 1;
        }
    };

}