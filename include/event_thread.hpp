#pragma once
#include "util.hpp"
#include "async_queue.hpp"
#include "loop_thread.hpp"
template <class E>
class EventThread : public LoopThread
{
  public:
    int init(size_t queue_size)
    {
        __LOG(debug, "EventThread::init is called with queue size is : " << queue_size);
        int rc = LoopThread::init();
        if (rc != 0)
        {
            __LOG(error, "loop thread start fail!");
            return rc;
        }
        event_queue_.reset(new AsyncQueue<SPSCQueue<E>>(queue_size));
        return event_queue_->init(loop(), this, on_event_internal);
    }

    void close_handles()
    {
        __LOG(debug, "close handles is called");
        LoopThread::close_handles();
        event_queue_->close_handles();
    }

    bool send_event_async(const E &event)
    {
        __LOG(debug, "EventThread::send_event_async is called");
        return event_queue_->enqueue(event);
    }

    virtual void on_event(const E &event) = 0;

  private:
    static void on_event_internal(uv_async_t *async)
    {
        __LOG(debug, "EventThread::on_event_internal is called");
        EventThread *thread = static_cast<EventThread *>(async->data);
        E event;
        while (thread->event_queue_->dequeue(event))
        {
            thread->on_event(event);
        }
    }

    std::unique_ptr<AsyncQueue<SPSCQueue<E>>> event_queue_;
};