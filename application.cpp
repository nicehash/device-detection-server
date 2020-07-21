#include "application.hpp"
#include "listener_impl.hpp"
#include "device_detection/device_detection.h"

void application::start(HWND hwnd, std::string &&devices) {
	m_workers = std::make_shared<workers::workers>(1);
	m_listener = std::make_shared<http::listener_impl>(m_workers->io_context(), 0, "127.0.0.1", 18000, std::move(devices), [hwnd]() {
		DWORD_PTR result;
		SendMessageTimeout(hwnd, WM_CLOSE, NULL, NULL, SMTO_NORMAL, 1, &result);
	});
	m_listener->start();
}

void application::stop() {
	m_listener->stop();
	m_workers->stop();
}