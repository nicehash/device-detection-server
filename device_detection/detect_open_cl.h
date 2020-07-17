#pragma once

#include <string>
#include <vector>

namespace detect_open_cl
{
	std::string get_open_cl_devices_json_str(bool pretty_print);
	void fill_amd_device_names(std::vector<std::string>& devices);
}

