#pragma once
#include "util.hpp"
#include "async_queue.hpp"
#include "loop_thread.hpp"
#include "spsc_queue.hpp"
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
        rc = event_queue_->init(loop(), this, on_event_internal);
        if (rc != 0)
        {
            __LOG(error, "event_queue init fail!");
            return rc;
        }
        event_priority_queue_.reset(new AsyncQueue<SPSCQueue<E>>(queue_size));
        rc = event_priority_queue_->init(loop(), this, on_priority_event_internal);
        if (rc != 0)
        {
            __LOG(error, "event_queue init fail!");
            return rc;
        }
        return rc;
    }

    void close_handles()
    {
        __LOG(debug, "close handles is called");
        LoopThread::close_handles();
        event_queue_->close_handles();
        event_priority_queue_->close_handles();
    }

    bool send_event_async(E &&event)
    {
        __LOG(debug, "EventThread::send_event_async is called");
        return event_queue_->enqueue(std::move(event));
    }
    bool send_priority_event_async(E &&event)
    {
        __LOG(debug, "EventThread::send_priority_event_async is called");
        return event_priority_queue_->enqueue(std::move(event));
    }

    virtual void on_event(const E &event) = 0;
    virtual void on_priority_event(const E &event) = 0;

  private:
    static void on_event_internal(uv_async_t *async)
    {
        __LOG(debug, "EventThread::on_event_internal is called");
        EventThread *thread = static_cast<EventThread *>(async->data);
        E event;
        while (thread->event_queue_->dequeue(event))
        {
            while (thread->event_priority_queue_->dequeue(event))
            {
                thread->on_priority_event(event);
            }
            thread->on_event(event);
        }
    }
    static void on_priority_event_internal(uv_async_t *async)
    {
        __LOG(debug, "EventThread::on_priority_event_internal is called");
        EventThread *thread = static_cast<EventThread *>(async->data);
        E event;
        while (thread->event_priority_queue_->dequeue(event))
        {
            thread->on_priority_event(event);
        }
    }
    std::unique_ptr<AsyncQueue<SPSCQueue<E>>> event_queue_;
    std::unique_ptr<AsyncQueue<SPSCQueue<E>>> event_priority_queue_;
};