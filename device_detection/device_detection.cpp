#include "device_detection.h"

#include "detect_amd_adl.h"
#include "detect_nvidia_nvml.h"
#include "json.hpp"

#include <Windows.h>
#include <iostream>
#include <vector>
#include <bitset>
#include <array>
#include <string>
#include <intrin.h>

int PhysicalProcessorCount()
{
    ULONG p;
    if (GetNumaHighestNodeNumber(&p))
        return (int)p + 1;
    else
        return 1;
}

std::string CPU_Brand() {
	std::string brand_;
	//int cpuInfo[4] = {-1};
	std::array<int, 4> cpui;

	// Calling __cpuid with 0x80000000 as the function_id argument
	// gets the number of the highest valid extended ID.
	__cpuid(cpui.data(), 0x80000000);
	const int nExIds_ = cpui[0];

	char brand[0x40];
	memset(brand, 0, sizeof(brand));
	std::vector<std::array<int, 4>> extdata_;

	for (int i = 0x80000000; i <= nExIds_; ++i)
	{
		__cpuidex(cpui.data(), i, 0);
		extdata_.push_back(cpui);
	}

	// Interpret CPU brand string if reported
	if (nExIds_ >= 0x80000004)
	{
		memcpy(brand, extdata_[2].data(), sizeof(cpui));
		memcpy(brand + 16, extdata_[3].data(), sizeof(cpui));
		memcpy(brand + 32, extdata_[4].data(), sizeof(cpui));
		brand_ = brand;
	}
	return brand_;
}

std::tuple<std::vector<std::string>, std::string> device_detection::detect_and_get_json_str() {
    std::vector<std::string> devices;
	const auto cpu_name = CPU_Brand();
	const auto is_amd_threadripper = cpu_name.find("Threadripper") != std::string::npos;
	const auto cpu_count = is_amd_threadripper ? 1 : PhysicalProcessorCount();
	
	for (auto i = 0; i < cpu_count; i++) devices.push_back(cpu_name);
    detect_amd_adl::fill_amd_device_names(devices);
    detect_nvidia_nvml::fill_nvidia_device_names(devices);
    nlohmann::json j = {
        { "devices", devices }
    };
    
    return { devices, j.dump() };
}
