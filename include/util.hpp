#pragma once
#include <signal.h>
#include "logger/logger.hpp"

#if __cplusplus >= 201703L
// use std::any
#include <any>
#define TASK_ANY std::any
#define TASK_ANY_CAST std::any_cast
#else
#include <boost/any.hpp>
#define TASK_ANY boost::any
#define TASK_ANY_CAST boost::any_cast
#endif

#define UNUSED_(X) ((void)X)

inline size_t next_pow_2(size_t num)
{
    size_t next = 2;
    size_t i = 0;
    while (next < num)
    {
        next = static_cast<size_t>(1) << i++;
    }
    return next;
}

// message tyep
enum class MSG_TYPE : unsigned int
{
    // for manager
    MANAGER_HB_REQ = 0,
    // for worker
    MANAGER_HB_RSP,
    HW
};



static int block_sigpipe()
{
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGPIPE);
    return pthread_sigmask(SIG_BLOCK, &set, NULL);
}

static void consume_blocked_sigpipe()
{
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGPIPE);
    struct timespec ts = {0, 0};
    int num = sigtimedwait(&set, NULL, &ts);
    if (num > 0)
    {
        __LOG(warn, "Caught and ignored SIGPIPE on loop thread");
    }
}
