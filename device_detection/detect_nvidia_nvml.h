#pragma once

#include <string>
#include <vector>

namespace detect_nvidia_nvml
{
	std::string get_nvidia_nvml_devices_json_str(bool pretty_print);
	void fill_nvidia_device_names(std::vector<std::string>& devices);
}

