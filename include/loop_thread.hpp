#pragma once
#include "util.hpp"
#include "mpmc_queue.hpp"
#include <signal.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <uv.h>


class LoopThread
{
  public:
    LoopThread() : is_loop_initialized_(false), is_joinable_(false)
    {
    }

    virtual ~LoopThread()
    {
        if (is_loop_initialized_)
        {
            uv_loop_close(&loop_);
        }
    }

    int init()
    {
        __LOG(debug, "LoopThread::init is called ");
        int rc = 0;

        rc = uv_loop_init(&loop_);

        if (rc != 0)
        {
            __LOG(error, "uv loop init fail!");
            return rc;
        }

        is_loop_initialized_ = true;

        rc = block_sigpipe();
        if (rc != 0)
        {
            __LOG(error, "block_sigpipe fail!");
            return rc;
        }
        rc = uv_prepare_init(loop(), &prepare_);
        if (rc != 0)
        {
            __LOG(error, "uv_prepare_init fail!");
            return rc;
        }
        rc = uv_prepare_start(&prepare_, on_prepare);
        if (rc != 0)
        {
            __LOG(error, "uv_prepare_start fail!");
            return rc;
        }

        return rc;
    }

    void close_handles()
    {
        uv_prepare_stop(&prepare_);
        uv_close(reinterpret_cast<uv_handle_t *>(&prepare_), NULL);
    }

    uv_loop_t *loop()
    {
        return &loop_;
    }

    int run()
    {
        int rc = uv_thread_create(&thread_, on_run_internal, this);
        if (rc == 0)
        {
            is_joinable_ = true;
            __LOG(debug, "start a new thread for uv loop!");
        }
        else
        {
            __LOG(error, "start a new thread for uv loop fail!");
        }
        return rc;
    }

    void join()
    {
        if (is_joinable_)
        {
            is_joinable_ = false;
            int rc = uv_thread_join(&thread_);
            UNUSED_(rc);
            assert(rc == 0);
        }
    }

  protected:
    virtual void on_run()
    {
        __LOG(debug, "LoopThread::on_run is called!");
    }
    virtual void on_after_run()
    {
        __LOG(debug, "LoopThread::on_after_run is called!");
    }

  private:
    static void on_run_internal(void *data)
    {
        __LOG(debug, "LoopThread::on_run_internal is called");
        LoopThread *thread = static_cast<LoopThread *>(data);
        thread->on_run();
        uv_run(thread->loop(), UV_RUN_DEFAULT);
        thread->on_after_run();
    }

    uv_loop_t loop_;
    bool is_loop_initialized_;

    static void on_prepare(uv_prepare_t *prepare)
    {
        __LOG(debug, "LoopThread::on_prepare is called");
        consume_blocked_sigpipe();
    }

    uv_prepare_t prepare_;

    uv_thread_t thread_;
    bool is_joinable_;
};