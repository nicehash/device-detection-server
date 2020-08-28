#pragma once

#include <string>
#include <cstdint>
#include <memory>
#include <functional>

namespace http {

class connection {
public:
    using close_cb = std::function<void(const std::shared_ptr<connection> &)>;

    virtual ~connection() = default;

    virtual void start() = 0;

    virtual void stop() = 0;

    virtual uint32_t index() const = 0;

    virtual bool got_quit() const = 0;
};

}


