#pragma once

#include <string>
#include <vector>

namespace detect_amd_adl
{
	std::string get_amd_adl_devices_json_str(bool pretty_print);
	void fill_amd_device_names(std::vector<std::string>& devices);
}

