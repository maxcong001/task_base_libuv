#pragma once

#include <uv.h>
#include <functional>
struct RepeatData
{
    uv_loop_t *loop;
    int count;
    void *data;
};

class Timer
{
  public:
    //typedef void (*Callback)(Timer *);
    using Callback = std::function<void(Timer *)>;
    typedef std::function<int(void *, int)> CBHandler;
    Timer(uv_loop_t *loop) : handle_(NULL), data_(NULL), cb_(NULL), round_left_(0), interval_(0), loop_(loop)
    {
    }
    Timer() = delete;

    ~Timer()
    {
        stop();
    }

    void *data() const { return data_; }

    bool is_running() const
    {
        if (handle_ == NULL)
            return false;
        return uv_is_active(reinterpret_cast<uv_handle_t *>(handle_)) != 0;
    }

    bool startOnce(uint64_t timeout, void *data, Callback cb)
    {
        return startRounds(timeout, 1, data, cb);
    }

    bool startRounds(uint64_t interval, uint64_t rounds, void *data, Callback cb)
    {
        __LOG(debug, "start rounds, handle is : " << (void *)handle_ << " round left : " << rounds);
        if (!handle_)
        {
            __LOG(debug, "handle_ is NULL, start a new one");
            handle_ = new uv_timer_t;
            handle_->data = this;
            uv_timer_init(loop_, handle_);
        }
        interval_ = interval;
        //loop_ = loop;
        data_ = data;
        cb_ = cb;
        round_left_ = rounds;
        uv_timer_start(handle_, on_timeout, interval, 0);
        return true;
    }
    static void on_timeout(uv_timer_t *handle)
    {
        Timer *timer = static_cast<Timer *>(handle->data);

        uint64_t _count_left = timer->dec_round_left();

        if (_count_left)
        {
            timer->startRounds(timer->interval_, _count_left, timer->data_, timer->cb_);
        }
        else
        {
            timer->stop();
        }
        timer->cb_(timer);
    }

    // note: actually not forever......, it works for life time ^_^
    bool startForever(uint64_t interval, void *data, Callback cb)
    {
        return startRounds(interval, uint64_t(-1), data, cb);
    }

    bool startAfter(uint64_t after, uint64_t interval, uint64_t round, void *data, Callback cb)
    {
        return startOnce(after, data, [=](Timer *timer) {
            timer->startRounds(interval, round, data, cb);
        });
    }

    bool startCB(uint32_t interval, CBHandler handler, void *userData, int tid)
    {
        int _tid = tid;
        void *_userData = userData;
        CBHandler _CBHandler = handler;
        return startForever(interval, NULL, [=](Timer *timer) {
            int ret = _CBHandler(_userData, _tid);
            if (ret == -1)
            {
                timer->stop();
            }
        });
    }

    void stop()
    {
        if (handle_ == NULL)
            return;
        // This also stops the timer
        uv_close(reinterpret_cast<uv_handle_t *>(handle_), on_close);
        handle_ = NULL;
    }

    static void on_close(uv_handle_t *handle)
    {
        delete reinterpret_cast<uv_timer_t *>(handle);
    }

    inline uint64_t dec_round_left()
    {
        return --round_left_;
    }
    inline uint64_t get_round_left()
    {
        return round_left_;
    }

  private:
    uv_timer_t *handle_;
    void *data_;
    Callback cb_;

    uint64_t round_left_;
    uint64_t interval_;
    uv_loop_t *loop_;
};
