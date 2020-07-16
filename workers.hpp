#pragma once

#include <memory>
#include <thread>
#include <string>
#include <boost/asio/io_context.hpp>
#include <boost/thread.hpp>

namespace workers {

class workers {
public:

    workers(int num_workers);


    void stop();

    boost::asio::io_context &io_context();

private:
    int worker(boost::asio::io_context &io_context);

    const int m_num_workers;
    bool m_running = true;

    boost::asio::io_context m_io_context;
    std::shared_ptr<boost::asio::io_context::work> m_work;
    std::shared_ptr<boost::thread_group> m_worker_threads;
};

}
