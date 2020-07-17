#include "application.hpp"
#include "listener_impl.hpp"
#include "device_detection/device_detection.h"

void application::start() {
	// ni videt static build
	// start/stop??? std::atomic
	auto detected_devs = device_detection::detect_and_get_json_str();
	m_workers = std::make_shared<workers::workers>(1);
	m_listener = std::make_shared<http::listener_impl>(m_workers->io_context(), 0, "127.0.0.1", 18000, detected_devs);
	m_listener->start();
}

void application::stop() {
	m_listener->stop();
	m_workers->stop();
}