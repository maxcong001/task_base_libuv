
#include "logger/logger.hpp"
#include "event_thread.hpp"
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

int main()
{
    set_log_level(logger_iface::log_level::debug);
    std::cout << "test started" << std::endl;

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
    //std::this_thread::sleep_for(std::chrono::milliseconds(500000));

    std::cout << "test end" << std::endl;
}
