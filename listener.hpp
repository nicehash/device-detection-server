#pragma once

#include <boost/system/error_code.hpp>
#include <functional>

namespace http
{
class listener{
public:
    using close_cb = std::function<void()>;

    virtual ~listener() = default;
    virtual boost::system::error_code start() = 0;
    virtual void stop() = 0;
};
}
