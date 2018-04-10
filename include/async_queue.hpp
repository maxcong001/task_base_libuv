#pragma once

#include <uv.h>

template <typename Q>
class AsyncQueue
{
public:
  AsyncQueue(size_t queue_size)
      : queue_(queue_size) {}

  int init(uv_loop_t *loop, void *data, uv_async_cb async_cb)
  {
    __LOG(debug, "AsyncQueue::init is called");
    async_.data = data;
    return uv_async_init(loop, &async_, async_cb);
  }

  void close_handles()
  {
    __LOG(debug, "AsyncQueue::close_handles is called");
    uv_close(reinterpret_cast<uv_handle_t *>(&async_), NULL);
  }

  void send()
  {
    uv_async_send(&async_);
  }

  bool enqueue(const typename Q::EntryType &data)
  {
    __LOG(debug, "AsyncQueue::enqueue is called");
    if (queue_.enqueue(data))
    {
      // uv_async_send() makes no guarantees about synchronization so it may
      // be necessary to use a memory fence to make sure stores happen before
      // the event loop wakes up and runs the async callback.
      Q::memory_fence();
      uv_async_send(&async_);
      return true;
    }
    return false;
  }

  bool dequeue(typename Q::EntryType &data)
  {
    __LOG(debug, "AsyncQueue::dequeue is called");
    return queue_.dequeue(data);
  }

  // Testing only
  bool is_empty() const { return queue_.is_empty(); }

private:
  uv_async_t async_;
  Q queue_;
};
