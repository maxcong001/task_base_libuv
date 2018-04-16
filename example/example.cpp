
#include "logger/logger.hpp"
#include "event_thread.hpp"
#include "timer.hpp"
#include <chrono>
#include <thread>
#include "spsc_queue.hpp"
struct TASK_MSG
{
    MSG_TYPE type;
    TASK_ANY body;
};

class IOWorker : public EventThread<TASK_MSG>
{
  public:
    IOWorker()
    {
        check_.data = this;
        prepare_.data = this;
    }
    virtual void on_event(const TASK_MSG &msg) override
    {
        __LOG(debug, "receive message with type : " << (char)msg.type);
    }
    virtual void on_priority_event(const TASK_MSG &msg) override
    {
        __LOG(debug, "receive priority message with type : " << (char)msg.type);
    }

    int init()
    {
        int rc = EventThread<TASK_MSG>::init(10);
        if (rc != 0)
        {
            __LOG(error, "EventThread<TASK_MSG>::init fail!");
            return rc;
        }
        rc = uv_check_init(loop(), &check_);
        if (rc != 0)
        {
            __LOG(error, "uv_check_init fail!");
            return rc;
        }
        rc = uv_check_start(&check_, on_check);
        if (rc != 0)
        {
            __LOG(error, "uv_check_start fail!");
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
            __LOG(error, "uv_prepare_start fail! rc is " << rc);
            return rc;
        }
        return rc;
    }
    static void on_check(uv_check_t *check)
    {
        __LOG(debug, "IOWorker::on_check is called");
    }
    static void on_prepare(uv_prepare_t *prepare)
    {
        __LOG(debug, "IOWorker::on_prepare is called");
    }

    uv_check_t check_;
    uv_prepare_t prepare_;
};

void timer_cb(Timer *)
{
    __LOG(debug, "timer fired");
}
void timer_once_cb(Timer *)
{
    __LOG(debug, "timer once fired");
}
void timer_after_cb(Timer *)
{
    __LOG(debug, "timer after fired");
}
int main()
{
    set_log_level(logger_iface::log_level::debug);
    std::cout << "test started" << std::endl;

    auto tmp_ptr = new IOWorker();

    TASK_MSG msg;
    msg.type = MSG_TYPE::MANAGER_HB_REQ;
    tmp_ptr->init();

    tmp_ptr->run();
#if 0
    for (int i = 0; i < 100; i++)
    {
        for (int j = 0; j < 10; j++)
        {
             tmp_ptr->send_event_async(std::move(msg));
        }
        tmp_ptr->send_priority_event_async(std::move(msg));
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
#endif
    for (int i = 0; i < 10; i++)
    {
       tmp_ptr->send_event_async(std::move(msg)); 
       std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(15000));

#if 0
    __LOG(debug, "now create a loop");
    uv_loop_t *loop = uv_loop_new();
    //uv_loop_t *loop = uv_default_loop();
    __LOG(debug, "create a loop successful");
    Timer timer(loop);
    __LOG(debug, "new a timer and will start rounds")
    timer.startRounds(500, 10, NULL, timer_cb);
    timer.stop();
    __LOG(warn, "now test start once");
    timer.startOnce(500, NULL, timer_once_cb);

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    __LOG(warn, "now test start after, should run after 5s ");
    timer.stop();
    timer.startAfter(1000, 4000, 2, NULL, timer_after_cb);

    std::thread loop_thread(uv_run, loop, UV_RUN_DEFAULT);
    std::this_thread::sleep_for(std::chrono::milliseconds(5));

    std::this_thread::sleep_for(std::chrono::milliseconds(20000));
    loop_thread.join();
#endif
#if 0
    using unique_ptr_tmp = std::unique_ptr<int>;
    auto tmp = new SPSCQueue<unique_ptr_tmp>(100);

    auto foo = std::unique_ptr<int>(new int(101));
    tmp->enqueue(std::move(foo));
    unique_ptr_tmp bar;
    tmp->dequeue(bar);
    __LOG(debug, "got form queue : " << *bar);
#endif
    std::cout << "test end" << std::endl;
}
