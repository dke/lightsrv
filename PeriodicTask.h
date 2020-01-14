#ifndef LIGHTSRV_PERIODICTASK_H
#define LIGHTSRV_PERIODICTASK_H

#include <boost/noncopyable.hpp>

void log_text(std::string const& text);

class PeriodicTask : boost::noncopyable
{
public:
    typedef std::function<void()> handler_fn;
    PeriodicTask(boost::asio::io_service& ioService, std::string const& name, int interval, handler_fn task, bool start_imm);
    void execute(boost::system::error_code const& e);
    void start();
private:
    void start_wait();
private:
    boost::asio::io_service& ioService;
    boost::asio::deadline_timer timer;
    handler_fn task;
    std::string name;
    int interval;
    bool start_imm;
};

#endif
