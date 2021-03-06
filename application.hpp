#pragma once

#include "workers.hpp"
#include "listener.hpp"
#include <memory>

class application
{
public:
    void start(HWND hwnd, std::string&& devices);
    void stop();

private:
    std::shared_ptr<workers::workers> m_workers;
    std::shared_ptr<http::listener> m_listener;

};



