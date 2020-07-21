#include "device_detection.h"

#include "detect_amd_adl.h"
#include "detect_nvidia_nvml.h"
#include "detect_open_cl.h"
#include "json.hpp"

std::string device_detection::detect_and_get_json_str() {
    std::vector<std::string> devices;
    detect_amd_adl::fill_amd_device_names(devices);
    detect_nvidia_nvml::fill_nvidia_device_names(devices);
    //detect_open_cl::fill_amd_device_names(devices);
    nlohmann::json j = {
        { "devices", devices }
    };
    return j.dump();
}
