
#include "logger/logger.hpp"
#include "event_thread.hpp"
#include "timer.hpp"
#include <chrono>
#include <thread>
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

#if 0
    auto tmp_ptr = new IOWorker();

    TASK_MSG msg;
    msg.type = MSG_TYPE::MANAGER_HB_REQ;
    tmp_ptr->init();
    tmp_ptr->run();

    for (int i = 0; i < 10000000; i++)
    {
        for (int j = 0; j < 1; j++)
        {
            bool ret = tmp_ptr->send_event_async(msg);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
#endif

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
    std::cout << "test end" << std::endl;
}
