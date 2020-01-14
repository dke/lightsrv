#include <syslog.h>

#include <iostream>
#include <functional>
#include <iomanip>

#include <boost/bind.hpp>

#include <boost/asio/io_service.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <boost/asio/placeholders.hpp>

#include "PeriodicTask.h"

PeriodicTask::PeriodicTask(boost::asio::io_service& ioService
    , std::string const& name
    , int interval
    , handler_fn task
    , bool start_imm)
    : ioService(ioService)
    , timer(ioService)
    , task(task)
    , name(name)
    , interval(interval)
    , start_imm(start_imm)

{
    syslog(LOG_INFO, "Create PeriodicTask '%s'", name.c_str());
    // Schedule start to be ran by the io_service
    ioService.post(boost::bind(&PeriodicTask::start, this));
}

void PeriodicTask::execute(boost::system::error_code const& e)
{
    if (e != boost::asio::error::operation_aborted) {
        syslog(LOG_INFO, "Execute PeriodicTask '%s'", name.c_str());

        task();

        timer.expires_at(timer.expires_at() + boost::posix_time::seconds(interval));
        start_wait();
    }
}

void PeriodicTask::start()
{
    syslog(LOG_INFO, "Start PeriodicTask '%s'", name.c_str());

    // Uncomment if you want to call the handler on startup (i.e. at time 0)
    if(start_imm) task();

    timer.expires_from_now(boost::posix_time::seconds(interval));
    start_wait();
}

void PeriodicTask::start_wait()
{
    timer.async_wait(boost::bind(&PeriodicTask::execute
        , this
        , boost::asio::placeholders::error));
}
