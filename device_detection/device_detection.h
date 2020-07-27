#pragma once

#include <string>
#include <tuple>
#include <vector>

namespace device_detection {
	std::tuple<std::vector<std::string>, std::string> detect_and_get_json_str();
}

