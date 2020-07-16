#include "workers.hpp"
#include "trace.hpp"

namespace workers {

workers::workers(int num_workers):
        m_num_workers(num_workers)
{
    m_work = std::make_shared<boost::asio::io_context::work>(m_io_context);

    m_worker_threads = std::make_shared<boost::thread_group>();
    for (int i = 0; i != m_num_workers; ++i) {
        m_worker_threads->create_thread(boost::bind(&workers::worker, this, std::ref(m_io_context)));
    }
}

boost::asio::io_context &workers::io_context()
{
    return m_io_context;
}

void workers::stop()
{
    m_running = false;
    m_io_context.stop();
    m_worker_threads->join_all();
}

int workers::worker(boost::asio::io_context &io_context)
{
    while (m_running) {
        try {
            boost::system::error_code ec;
            io_context.run(ec);

            if (ec) {
                TRACE(L"Error:" << ec.message().c_str());
            }
        }
        catch (std::exception &ex) {
            TRACE("Exception '" << ex.what() << "'");
        }
        catch (boost::system::error_code &ec) {
            TRACE("Exception: " << ec.message().c_str());
        }
    }

    return 0;
}

}
