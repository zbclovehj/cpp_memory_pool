#pragma once
#include "Common.h"
namespace Memory_Pool
{
    class ThreadCache
    {
    public:
        // 线程单例模式
        static ThreadCache *getInstance()
        {
            /*
            static：确保 instance 变量在程序的整个生命周期内只初始化一次。
thread_local：这是 C++11 引入的关键字，用于声明线程局部变量。
线程局部变量意味着每个线程都有自己独立的该变量副本，
不同线程之间的副本相互独立，
一个线程对其副本的修改不会影响其他线程的副本
            */
            static thread_local ThreadCache instance;
            return &instance;
        }
        // 分配指定大小的内存
        void *allocate(size_t size);
        // 释放内存,通过size来获取大小
        void deallocate(void *ptr, size_t size);

    private:
        // 固定大小容器，编译时确定大小
        std::array<void *, FREE_LIST_SIZE> freeList_;
        std::array<size_t, FREE_LIST_SIZE> freeListSize_;
        ThreadCache()
        {
            freeList_.fill(nullptr);
            freeListSize_.fill(0);
        }
        // 从上一级缓存获取内存
        void *fetchFromCentralCache(size_t index);
        // 归还内存至上一级
        void returnToCentralCache(void *start, size_t size);
        bool shouldReturnToCentralCache(size_t index);
        size_t getBatchNum(size_t size);
    };
} // namespace name
