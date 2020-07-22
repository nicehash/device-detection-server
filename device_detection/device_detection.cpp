#include "device_detection.h"

#include "detect_amd_adl.h"
#include "detect_nvidia_nvml.h"
#include "json.hpp"

std::tuple<std::string, std::string> device_detection::detect_and_get_json_str() {
    std::vector<std::string> devices;
    detect_amd_adl::fill_amd_device_names(devices);
    detect_nvidia_nvml::fill_nvidia_device_names(devices);
    nlohmann::json j = {
        { "devices", devices }
    };
    std::string devices_list = "No devices are detected.";
    if (devices.size() > 0) {
        devices_list = "Detected devices:\n\r";
        for (int i = 0; i < devices.size() - 1; i++) devices_list += "\t- #" + std::to_string(i+1) + " " + devices[i] + ",\n\r";
        devices_list += "\t- #" + std::to_string(devices.size()) + " " + devices[devices.size() - 1] + ".";
    }
    return { devices_list, j.dump() };
}
