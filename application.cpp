#include "application.hpp"
#include "listener_impl.hpp"


void application::start() {
	m_workers = std::make_shared<workers::workers>(1);
	m_listener = std::make_shared<http::listener_impl>(m_workers->io_context(), 0, "127.0.0.1", 18000);
	m_listener->start();

}

void application::stop() {
	m_listener->stop();
	m_workers->stop();
}