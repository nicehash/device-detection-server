#pragma once

#include <boost/system/error_code.hpp>

namespace http
{
class listener{
public:
    virtual ~listener() = default;
    virtual boost::system::error_code start() = 0;
    virtual void stop() = 0;
};
}
