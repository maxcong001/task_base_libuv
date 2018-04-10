/*
  Implementation of Dmitry Vyukov's MPMC algorithm
  http://www.1024cores.net/home/lock-free-algorithms/queues/bounded-mpmc-queue
*/

#include <atomic>
#include <assert.h>
#include <cstddef>
#include "util.hpp"

template <typename T>
class MPMCQueue
{
  public:
    typedef T EntryType;

    MPMCQueue(size_t size)
        : size_(next_pow_2(size)), mask_(size_ - 1), buffer_(new Node[size_]), tail_(0), head_(0)
    {
        // populate the sequence initial values
        for (size_t i = 0; i < size_; ++i)
        {
            buffer_[i].seq.store(i, std::memory_order_relaxed);
        }
    }

    ~MPMCQueue() { delete[] buffer_; }

    bool enqueue(const T &data)
    {
        // head_seq_ only wraps at MAX(head_seq_) instead we use a mask to
        // convert the sequence to an array index this is why the ring
        // buffer must be a size which is a power of 2. this also allows
        // the sequence to double as a ticket/lock.
        size_t pos = tail_.load(std::memory_order_relaxed);

        for (;;)
        {
            Node *node = &buffer_[pos & mask_];
            size_t node_seq = node->seq.load(std::memory_order_acquire);
            intptr_t dif = (intptr_t)node_seq - (intptr_t)pos;

            // if seq and head_seq are the same then it means this slot is empty
            if (dif == 0)
            {
                // claim our spot by moving head if head isn't the same as we
                // last checked then that means someone beat us to the punch
                // weak compare is faster, but can return spurious results
                // which in this instance is OK, because it's in the loop
                if (tail_.compare_exchange_weak(pos, pos + 1, std::memory_order_relaxed))
                {
                    // set the data
                    node->data = data;
                    // increment the sequence so that the tail knows it's accessible
                    node->seq.store(pos + 1, std::memory_order_release);
                    return true;
                }
            }
            else if (dif < 0)
            {
                // if seq is less than head seq then it means this slot is
                // full and therefore the buffer is full
                return false;
            }
            else
            {
                // under normal circumstances this branch should never be taken
                pos = tail_.load(std::memory_order_relaxed);
            }
        }

        // never taken
        return false;
    }

    bool dequeue(T &data)
    {
        size_t pos = head_.load(std::memory_order_relaxed);

        for (;;)
        {
            Node *node = &buffer_[pos & mask_];
            size_t node_seq = node->seq.load(std::memory_order_acquire);
            intptr_t dif = (intptr_t)node_seq - (intptr_t)(pos + 1);

            // if seq and head_seq are the same then it means this slot is empty
            if (dif == 0)
            {
                // claim our spot by moving head if head isn't the same as we
                // last checked then that means someone beat us to the punch
                // weak compare is faster, but can return spurious results
                // which in this instance is OK, because it's in the loop
                if (head_.compare_exchange_weak(pos, pos + 1, std::memory_order_relaxed))
                {
                    // set the output
                    data = node->data;
                    // set the sequence to what the head sequence should be next
                    // time around
                    node->seq.store(pos + mask_ + 1, std::memory_order_release);
                    return true;
                }
            }
            else if (dif < 0)
            {
                // if seq is less than head seq then it means this slot is
                // full and therefore the buffer is full
                return false;
            }
            else
            {
                // under normal circumstances this branch should never be taken
                pos = head_.load(std::memory_order_relaxed);
            }
        }

        // never taken
        return false;
    }

    bool is_empty() const
    {
        size_t pos = head_.load(std::memory_order_acquire);
        Node *node = &buffer_[pos & mask_];
        size_t node_seq = node->seq.load(std::memory_order_acquire);
        return (intptr_t)node_seq - (intptr_t)(pos + 1) < 0;
    }

    static void memory_fence()
    {
        std::atomic_thread_fence(std::memory_order_seq_cst);
    }

  private:
    struct Node
    {
        std::atomic<size_t> seq;
        T data;
    };

    // it's either 32 or 64 so 64 is good enough
    typedef char CachePad[64];

    CachePad pad0_;
    const size_t size_;
    const size_t mask_;
    Node *const buffer_;
    CachePad pad1_;
    std::atomic<size_t> tail_;
    CachePad pad2_;
    std::atomic<size_t> head_;
    CachePad pad3_;
};