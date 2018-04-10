/*
  Note:
  A combination of the algorithms described by the circular buffers
  documentation found in the Linux kernel, and the bounded MPMC queue
  by Dmitry Vyukov[1]. Implemented in pure C++11. Should work across
  most CPU architectures.

  [1]
  http://www.1024cores.net/home/lock-free-algorithms/queues/bounded-mpmc-queue
*/
#pragma once

#include <atomic>
#include <assert.h>
#include <cstddef>
#include "util.hpp"
#include <utility>

template <typename T>
class SPSCQueue
{
  public:
    typedef T EntryType;

    SPSCQueue(size_t size)
        : size_(next_pow_2(size)), mask_(size_ - 1), buffer_(new T[size_]), tail_(0), head_(0) {}

    ~SPSCQueue() { delete[] buffer_; }

    bool enqueue(T &&input)
    {
        size_t pos = tail_.load(std::memory_order_relaxed);
        size_t next_pos = (pos + 1) & mask_;
        if (next_pos == head_.load(std::memory_order_acquire))
        {
            return false;
        }
        buffer_[pos] = std::move(input);
        tail_.store(next_pos, std::memory_order_release);
        return true;
    }

    bool dequeue(T &output)
    {
        const size_t pos = head_.load(std::memory_order_relaxed);
        if (pos == tail_.load(std::memory_order_acquire))
        {
            return false;
        }
        output = std::move(buffer_[pos]);
        head_.store((pos + 1) & mask_, std::memory_order_release);
        return true;
    }

    bool is_empty()
    {
        return head_.load(std::memory_order_acquire) ==
               tail_.load(std::memory_order_acquire);
    }

    static void memory_fence()
    {
        // Internally, libuv has a "pending" flag check whose load can be reordered
        // before storing the data into the queue causing the data in the queue
        // not to be consumed. This fence ensures that the load happens after the
        // data has been store in the queue.

        std::atomic_thread_fence(std::memory_order_seq_cst);
    }

  private:
    typedef char cache_line_pad_t[64];

    cache_line_pad_t pad0_;
    const size_t size_;
    const size_t mask_;
    T *const buffer_;

    cache_line_pad_t pad1_;
    std::atomic<size_t> tail_;

    cache_line_pad_t pad2_;
    std::atomic<size_t> head_;
};
